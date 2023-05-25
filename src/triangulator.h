#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "heightmap.h"

class Triangulator
{
public:
    Triangulator(
        const std::shared_ptr<Heightmap>& heightmap,
        float error, int nTri, int nVert);

    void Initialize();
    void RunStep();
    void ReverseStep();
    void Run();

    int NumPoints() const
    {
        return m_Points.size();
    }

    int NumTriangles() const
    {
        return m_Queue.size();
    }

    float Error() const;

    std::vector<glm::vec3> Points(const float zScale) const;

    std::vector<glm::ivec3> Triangles() const;

private:
    void Flush();

    void Step();

    int AddPoint(const glm::ivec2 point);

    int AddTriangle(
        const int a, const int b, const int c,
        const int ab, const int bc, const int ca,
        int e);

    void Legalize(const int a);

    void QueuePush(const int t);
    int QueuePop();
    int QueuePopBack();
    void QueueRemove(const int t);
    bool QueueLess(const int i, const int j) const;
    void QueueSwap(const int i, const int j);
    void QueueUp(const int j0);
    bool QueueDown(const int i0, const int n);

    void Snapshot();

    std::shared_ptr<Heightmap> m_Heightmap;

    std::vector<glm::ivec2> m_Points;
    std::vector<int> m_Triangles;
    std::vector<int> m_Halfedges;
    std::vector<glm::ivec2> m_Candidates;
    std::vector<float> m_Errors;
    std::vector<int> m_QueueIndexes;
    std::vector<int> m_Queue;
    std::vector<int> m_Pending;

    std::vector<glm::ivec2> m_PointsTmp;
    std::vector<int> m_TrianglesTmp;
    std::vector<int> m_HalfedgesTmp;
    std::vector<glm::ivec2> m_CandidatesTmp;
    std::vector<float> m_ErrorsTmp;
    std::vector<int> m_QueueIndexesTmp;
    std::vector<int> m_QueueTmp;
    std::vector<int> m_PendingTmp;

    const float m_MaxError;
    const int m_MaxTriangles;
    const int m_MaxPoints;
};
