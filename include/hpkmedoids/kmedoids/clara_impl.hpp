#pragma once

#include <mpi.h>

#include <functional>
#include <hpkmedoids/kmedoids/kmedoids.hpp>
#include <hpkmedoids/utils/sampler.hpp>

namespace hpkmedoids
{
int32_t defaultSampleSize(const int32_t numData, const int32_t numClusters) { return 40 + 2 * numClusters; }

template <typename T, Parallelism Level, class DistanceFunc>
class CLARAKMedoidsImpl : public KMedoids<T, Level, DistanceFunc>
{
public:
    CLARAKMedoidsImpl(const std::string& initializer, const std::string& maximizer,
                      std::function<int32_t(const int32_t, const int32_t)> sampleSizeCalc) :
        KMedoids<T, Level, DistanceFunc>(initializer, maximizer), m_sampleSizeCalc(sampleSizeCalc)
    {
    }

    virtual ~CLARAKMedoidsImpl() = default;

    virtual const Clusters<T>* const fit(const Matrix<T>* const data, const int& numClusters, const int& numRepeats,
                                         const int numSamplingIters) = 0;

    const Clusters<T>* const getResults() const { return &m_bestNonSampledClusters; }

    virtual void reset() override
    {
        m_bestNonSampledClusters = Clusters<T>();
        KMedoids<T, Level, DistanceFunc>::reset();
    }

protected:
    Sampler<T> m_sampler;
    Clusters<T> m_bestNonSampledClusters;
    DistanceFunc m_distanceFunc;
    std::function<int32_t(const int32_t, const int32_t)> m_sampleSizeCalc;
};

template <typename T, Parallelism Level, class DistanceFunc>
class SharedMemoryCLARAKMedoids : public CLARAKMedoidsImpl<T, Level, DistanceFunc>
{
public:
    SharedMemoryCLARAKMedoids(const std::string& initializer, const std::string& maximizer,
                              std::function<int32_t(const int32_t, const int32_t)> sampleSizeCalc) :
        CLARAKMedoidsImpl<T, Level, DistanceFunc>(initializer, maximizer, sampleSizeCalc)
    {
    }

    const Clusters<T>* const fit(const Matrix<T>* const data, const int& numClusters, const int& numRepeats,
                                 const int numSamplingIters) override
    {
        auto sampleSize = this->m_sampleSizeCalc(data->rows(), numClusters);

        for (int i = 0; i < numSamplingIters; ++i)
        {
            auto sampledData = this->m_sampler.template sample<Level>(sampleSize, data);
            KMedoids<T, Level, DistanceFunc>::fit(&sampledData, numClusters, numRepeats);
            Clusters<T> clusters(data, this->m_bestClusters.getCentroids());
            clusters.template calculateAssignmentsFromCentroids<Level, DistanceFunc>(this->m_distanceFunc);
            this->compareResults(clusters, this->m_bestNonSampledClusters);
        }

        return this->getResults();
    }
};

template <typename T, Parallelism Level, class DistanceFunc>
class DistributedCLARAKMedoids : public CLARAKMedoidsImpl<T, Level, DistanceFunc>
{
public:
    DistributedCLARAKMedoids(const std::string& initializer, const std::string& maximizer,
                             std::function<int32_t(const int32_t, const int32_t)> sampleSizeCalc) :
        CLARAKMedoidsImpl<T, Level, DistanceFunc>(initializer, maximizer, sampleSizeCalc),
        m_rank(-1),
        m_size(-1),
        m_blank(0),
        m_samplesIssued(0),
        m_dtype(matchMPIType<T>())
    {
        MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &m_size);
    }

    const Clusters<T>* const fit(const Matrix<T>* const data, const int& numClusters, const int& numRepeats,
                                 const int numSamplingIters) override
    {
        int numCols     = data->cols();
        auto sampleSize = this->m_sampleSizeCalc(data->rows(), numClusters);

        MPI_Bcast(&numCols, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
        MPI_Bcast(&sampleSize, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

        if (m_rank == MASTER)
            master(data, numClusters, sampleSize, numSamplingIters);
        else
            worker(numCols, numClusters, sampleSize);

        MPI_Barrier(MPI_COMM_WORLD);

        return this->getResults();
    }

    void reset() override
    {
        m_samplesIssued = 0;
        CLARAKMedoidsImpl<T, Level, DistanceFunc>::reset();
    }

private:
    void master(const Matrix<T>* const data, const int32_t numClusters, const int32_t sampleSize,
                const int numSamplingIters)
    {
        Matrix<T> centroidBuffer(numClusters, data->cols(), true);

        while (m_samplesIssued < numSamplingIters)
        {
            MPI_Status status;
            MPI_Recv(&m_blank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == REQUEST_TAG)
                allocateWork(data, sampleSize, status);
            else if (status.MPI_TAG == COMPLETED_TAG)
            {
                processResults(data, &centroidBuffer, status);
                if (m_samplesIssued < numSamplingIters)
                    allocateWork(data, sampleSize, status);
            }
        }

        terminate(data, &centroidBuffer);
    }

    void terminate(const Matrix<T>* const data, Matrix<T>* const centroidBuffer)
    {
        for (int i = 1; i < m_size; ++i)
        {
            MPI_Status status;
            MPI_Recv(&m_blank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == COMPLETED_TAG)
                processResults(data, centroidBuffer, status);
            MPI_Send(&m_blank, 1, MPI_INT, status.MPI_SOURCE, TERMINATE_TAG, MPI_COMM_WORLD);
        }
    }

    void allocateWork(const Matrix<T>* const data, const int32_t sampleSize, const MPI_Status& status)
    {
        auto sampledData = this->m_sampler.template sample<Level>(sampleSize, data);
        MPI_Send(&m_blank, 1, MPI_INT, status.MPI_SOURCE, REQUEST_TAG, MPI_COMM_WORLD);
        MPI_Send(sampledData.data(), sampledData.size(), m_dtype, status.MPI_SOURCE, REQUEST_TAG, MPI_COMM_WORLD);
        ++m_samplesIssued;
    }

    void processResults(const Matrix<T>* const data, Matrix<T>* const centroids, const MPI_Status& status)
    {
        MPI_Recv(centroids->data(), centroids->size(), m_dtype, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        Clusters<T> clusters(data, centroids);
        clusters.template calculateAssignmentsFromCentroids<Level, DistanceFunc>(this->m_distanceFunc);
        this->compareResults(clusters, this->m_bestNonSampledClusters);
    }

    void worker(const int numCols, const int numClusters, const int sampleSize)
    {
        Matrix<T> sampledData(sampleSize, numCols, true);

        MPI_Send(&m_blank, 1, MPI_INT, MASTER, REQUEST_TAG, MPI_COMM_WORLD);

        while (true)
        {
            MPI_Status status;
            MPI_Recv(&m_blank, 1, MPI_INT, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TERMINATE_TAG)
                break;

            MPI_Recv(sampledData.data(), sampledData.size(), m_dtype, MASTER, REQUEST_TAG, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            KMedoids<T, Level, DistanceFunc>::fit(&sampledData, numClusters, 1);

            auto centroids = this->m_bestClusters.getCentroids();
            MPI_Send(&m_blank, 1, MPI_INT, MASTER, COMPLETED_TAG, MPI_COMM_WORLD);
            MPI_Send(centroids->data(), centroids->size(), m_dtype, MASTER, COMPLETED_TAG, MPI_COMM_WORLD);
        }
    }

private:
    const int MASTER        = 0;
    const int REQUEST_TAG   = 1;
    const int COMPLETED_TAG = 2;
    const int TERMINATE_TAG = 3;

    int m_rank;
    int m_size;
    int m_blank;
    int m_samplesIssued;
    MPI_Datatype m_dtype;
};
}  // namespace hpkmedoids