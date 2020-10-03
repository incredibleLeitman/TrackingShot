#pragma once

struct CameraWaypoint
{
    glm::vec3 position;
    glm::quat rotation;
};

class CameraPath
{
private:
    std::vector<CameraWaypoint> positions;

public:
    CameraPath()
    {
    }

    void AddPosition(CameraWaypoint pos)
    {
        positions.push_back(pos);
    }

    size_t PositionsSize()
    {
        return positions.size();
    }

    std::vector<CameraWaypoint>& Positions()
    {
        return positions;
    }
};
