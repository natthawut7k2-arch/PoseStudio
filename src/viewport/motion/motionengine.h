#ifndef MOTION_ENGINE_H
#define MOTION_ENGINE_H

#include "proceduralgait.h"
#include "trajectorysolver.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace pose {

struct MotionClip {
    GaitType    gait = GaitType::Walk;
    std::string name = "walk";
    float       duration = 5.0f;
    float       speed = 1.2f;
    float       frequency = 1.8f;
};

struct MotionFrame {
    glm::vec3 worldPosition = glm::vec3(0.0f);
    float     yawDegrees = 0.0f;
    std::vector<std::pair<std::string, glm::vec3>> boneRotations;
    bool      cameraFollow = true;
    bool      valid = false;
};

class MotionEngine {
public:
    MotionEngine();

    /// Loads a declarative motion choreography script from a JSON string.
    bool loadScriptFromJson(const std::string& jsonContent);

    /// Loads a script from a file path.
    bool loadScriptFromFile(const std::string& filePath);

    /// Transport controls
    void play();
    void pause();
    void stop();
    void seek(float timeSeconds);

    void setLoop(bool loop);
    void setCameraFollow(bool follow);

    bool isPlaying() const { return m_playing; }
    bool isLoop() const { return m_loop; }
    bool isCameraFollow() const { return m_cameraFollow; }

    float currentTime() const { return m_currentTime; }
    float totalDuration() const { return m_totalDuration; }
    std::string currentMotionName() const;

    /// Advances playback by dt seconds and evaluates the posed frame.
    MotionFrame evaluate(float dt);

    /// Evaluates at an arbitrary point in time without advancing playback state.
    MotionFrame evaluateAt(float time);

private:
    void recomputeTimeline();

    std::vector<MotionClip> m_clips;
    PathConfig              m_pathConfig;
    std::string             m_scriptName = "Motion Script";

    float m_currentTime = 0.0f;
    float m_totalDuration = 0.0f;
    float m_distanceTraveled = 0.0f;
    bool  m_playing = false;
    bool  m_loop = true;
    bool  m_cameraFollow = true;
};

} // namespace pose

#endif // MOTION_ENGINE_H
