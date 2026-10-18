#include "motionengine.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <algorithm>
#include <cmath>

namespace pose {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTransitionDuration = 0.5f; // cross-fade blend window in seconds

float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
} // namespace

MotionEngine::MotionEngine() {
    GaitParams walkParams;
    walkParams.forwardLean = 3.0f;
    walkParams.armSwing = 22.0f;
    walkParams.elbowBend = 18.0f;
    walkParams.kneeBend = 46.0f;
    walkParams.kneeStance = 8.0f;
    walkParams.torsoTwist = 4.0f;

    GaitParams runParams;
    runParams.forwardLean = 15.0f;
    runParams.armSwing = 40.0f;
    runParams.elbowBend = 70.0f;
    runParams.kneeBend = 72.0f;
    runParams.kneeStance = 18.0f;
    runParams.torsoTwist = 7.0f;

    m_clips = {
        {GaitType::Walk, "walk", 5.0f, 1.2f, 1.8f, walkParams},
        {GaitType::Run,  "run",  5.0f, 2.5f, 3.0f, runParams}
    };
    m_pathConfig.type = PathType::Circle;
    m_pathConfig.radius = 3.0f;
    m_pathConfig.center = glm::vec3(0.0f);
    m_pathConfig.loop = true;
    recomputeTimeline();
}

bool MotionEngine::loadScriptFromJson(const std::string& jsonContent) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray::fromRawData(jsonContent.data(), static_cast<int>(jsonContent.size())), &error);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    m_scriptName = root.value(QStringLiteral("name")).toString(QStringLiteral("Motion Script")).toStdString();

    if (root.contains(QStringLiteral("camera_follow"))) {
        m_cameraFollow = root.value(QStringLiteral("camera_follow")).toBool(true);
    }

    if (root.contains(QStringLiteral("path")) && root[QStringLiteral("path")].isObject()) {
        QJsonObject pathObj = root[QStringLiteral("path")].toObject();
        std::string ptype = pathObj.value(QStringLiteral("type")).toString(QStringLiteral("circle")).toStdString();
        m_pathConfig.type = TrajectorySolver::parsePathType(ptype);
        m_pathConfig.radius = static_cast<float>(pathObj.value(QStringLiteral("radius")).toDouble(3.0));
        m_pathConfig.loop = pathObj.value(QStringLiteral("loop")).toBool(true);

        if (pathObj.contains(QStringLiteral("center")) && pathObj[QStringLiteral("center")].isArray()) {
            QJsonArray centerArr = pathObj[QStringLiteral("center")].toArray();
            if (centerArr.size() >= 3) {
                m_pathConfig.center.x = static_cast<float>(centerArr[0].toDouble(0.0));
                m_pathConfig.center.y = static_cast<float>(centerArr[1].toDouble(0.0));
                m_pathConfig.center.z = static_cast<float>(centerArr[2].toDouble(0.0));
            }
        }
    }

    if (!root.contains(QStringLiteral("motion_sequence")) || !root[QStringLiteral("motion_sequence")].isArray()) {
        return false;
    }
    QJsonArray seq = root[QStringLiteral("motion_sequence")].toArray();
    if (seq.isEmpty()) {
        return false;
    }

    std::vector<MotionClip> parsedClips;

    if (root.contains(QStringLiteral("biomechanics")) && root[QStringLiteral("biomechanics")].isObject()) {
        QJsonObject bio = root[QStringLiteral("biomechanics")].toObject();
        GaitType gait = GaitType::Walk;
        std::string name = m_scriptName;
        std::string nameLower = name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        if (nameLower.find("run") != std::string::npos || nameLower.find("sprint") != std::string::npos || nameLower.find("dash") != std::string::npos) {
            gait = GaitType::Run;
        }

        float speed = static_cast<float>(bio.value(QStringLiteral("speed")).toDouble(ProceduralGait::defaultSpeed(gait)));
        float freq  = static_cast<float>(bio.value(QStringLiteral("frequency")).toDouble(ProceduralGait::defaultFrequency(gait)));
        float duration = 10.0f;

        GaitParams params;
        params.forwardLean = static_cast<float>(bio.value(QStringLiteral("forward_lean")).toDouble(3.0));
        params.armSwing    = static_cast<float>(bio.value(QStringLiteral("arm_swing")).toDouble(22.0));
        params.elbowBend   = static_cast<float>(bio.value(QStringLiteral("elbow_bend")).toDouble(20.0));
        params.kneeBend    = static_cast<float>(bio.value(QStringLiteral("knee_bend")).toDouble(48.0));
        params.kneeStance  = static_cast<float>(bio.value(QStringLiteral("knee_stance")).toDouble(8.0));
        params.torsoTwist  = static_cast<float>(bio.value(QStringLiteral("torso_twist")).toDouble(5.0));

        if (seq[0].isObject() && seq[0].toObject().contains(QStringLiteral("duration"))) {
            duration = static_cast<float>(seq[0].toObject().value(QStringLiteral("duration")).toDouble(10.0));
        }

        parsedClips.push_back({gait, name, duration, speed, freq, params});
    } else {
        for (const QJsonValue& val : seq) {
            if (!val.isObject()) continue;
            QJsonObject obj = val.toObject();
            std::string motionName = obj.value(QStringLiteral("motion")).toString(QStringLiteral("walk")).toStdString();
            GaitType gait = ProceduralGait::parseGaitType(motionName);
            float duration = static_cast<float>(obj.value(QStringLiteral("duration")).toDouble(5.0));
            float speed = static_cast<float>(obj.value(QStringLiteral("speed")).toDouble(ProceduralGait::defaultSpeed(gait)));
            float freq = static_cast<float>(obj.value(QStringLiteral("frequency")).toDouble(ProceduralGait::defaultFrequency(gait)));

            GaitParams params;
            if (obj.contains(QStringLiteral("forward_lean"))) {
                params.forwardLean = static_cast<float>(obj.value(QStringLiteral("forward_lean")).toDouble(-1.0));
            }
            if (obj.contains(QStringLiteral("arm_swing"))) {
                params.armSwing = static_cast<float>(obj.value(QStringLiteral("arm_swing")).toDouble(-1.0));
            }
            if (obj.contains(QStringLiteral("elbow_bend"))) {
                params.elbowBend = static_cast<float>(obj.value(QStringLiteral("elbow_bend")).toDouble(-1.0));
            }
            if (obj.contains(QStringLiteral("knee_bend"))) {
                params.kneeBend = static_cast<float>(obj.value(QStringLiteral("knee_bend")).toDouble(-1.0));
            }
            if (obj.contains(QStringLiteral("knee_stance"))) {
                params.kneeStance = static_cast<float>(obj.value(QStringLiteral("knee_stance")).toDouble(-1.0));
            }
            if (obj.contains(QStringLiteral("torso_twist"))) {
                params.torsoTwist = static_cast<float>(obj.value(QStringLiteral("torso_twist")).toDouble(-1.0));
            }

            parsedClips.push_back({gait, motionName, std::max(0.1f, duration), speed, freq, params});
        }
    }

    if (parsedClips.empty()) {
        return false;
    }
    m_clips = std::move(parsedClips);

    recomputeTimeline();
    seek(0.0f);
    return true;
}

bool MotionEngine::loadScriptFromFile(const std::string& filePath) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray bytes = file.readAll();
    return loadScriptFromJson(bytes.toStdString());
}

void MotionEngine::recomputeTimeline() {
    m_totalDuration = 0.0f;
    for (const auto& clip : m_clips) {
        m_totalDuration += clip.duration;
    }
}

void MotionEngine::play() {
    m_playing = true;
}

void MotionEngine::pause() {
    m_playing = false;
}

void MotionEngine::stop() {
    m_playing = false;
    m_distanceTraveled = 0.0f;
    seek(0.0f);
}

void MotionEngine::seek(float timeSeconds) {
    if (m_totalDuration > 0.0f) {
        m_currentTime = std::clamp(timeSeconds, 0.0f, m_totalDuration);
    } else {
        m_currentTime = 0.0f;
    }
    m_distanceTraveled = computeDistanceAt(m_currentTime);
}

float MotionEngine::computeDistanceAt(float time) const {
    if (m_clips.empty() || m_totalDuration <= 0.0f) return 0.0f;
    float t = std::clamp(time, 0.0f, m_totalDuration);
    float distance = 0.0f;
    float tAccum = 0.0f;
    for (size_t i = 0; i < m_clips.size(); ++i) {
        float clipDuration = m_clips[i].duration;
        if (t >= tAccum && t <= (tAccum + clipDuration)) {
            distance += m_clips[i].speed * (t - tAccum);
            break;
        } else {
            distance += m_clips[i].speed * clipDuration;
            tAccum += clipDuration;
        }
    }
    return distance;
}

void MotionEngine::setLoop(bool loop) {
    m_loop = loop;
}

void MotionEngine::setCameraFollow(bool follow) {
    m_cameraFollow = follow;
}

std::string MotionEngine::currentMotionName() const {
    if (m_clips.empty()) return "none";
    float tAccum = 0.0f;
    for (const auto& clip : m_clips) {
        if (m_currentTime >= tAccum && m_currentTime < (tAccum + clip.duration)) {
            return clip.name;
        }
        tAccum += clip.duration;
    }
    return m_clips.back().name;
}

MotionFrame MotionEngine::evaluate(float dt) {
    if (m_playing && !m_clips.empty() && m_totalDuration > 0.0f) {
        float activeSpeed = 1.2f;
        float tAccum = 0.0f;
        for (const auto& clip : m_clips) {
            if (m_currentTime >= tAccum && m_currentTime <= (tAccum + clip.duration)) {
                activeSpeed = clip.speed;
                break;
            }
            tAccum += clip.duration;
        }

        m_distanceTraveled += activeSpeed * dt;
        m_currentTime += dt;
        if (m_currentTime >= m_totalDuration) {
            if (m_loop) {
                m_currentTime = std::fmod(m_currentTime, m_totalDuration);
            } else {
                m_currentTime = m_totalDuration;
                m_playing = false;
            }
        }
    }
    return evaluateFrame(m_currentTime, m_distanceTraveled);
}

MotionFrame MotionEngine::evaluateAt(float time) {
    return evaluateFrame(time, computeDistanceAt(time));
}

MotionFrame MotionEngine::evaluateFrame(float time, float distance) {
    MotionFrame frame;
    frame.cameraFollow = m_cameraFollow;
    if (m_clips.empty() || m_totalDuration <= 0.0f) {
        frame.valid = false;
        return frame;
    }

    float t = std::clamp(time, 0.0f, m_totalDuration);

    float tAccum = 0.0f;
    size_t activeIndex = 0;
    float localT = 0.0f;

    for (size_t i = 0; i < m_clips.size(); ++i) {
        float clipDuration = m_clips[i].duration;
        if (t >= tAccum && t <= (tAccum + clipDuration)) {
            activeIndex = i;
            localT = t - tAccum;
            break;
        } else {
            tAccum += clipDuration;
        }
    }

    const MotionClip& currentClip = m_clips[activeIndex];
    float phase = 2.0f * kPi * currentClip.frequency * t;

    GaitFrame gaitFrame;
    // Check for transition blending to next clip if near boundary
    float timeRemainingInClip = currentClip.duration - localT;
    bool hasNextClip = (activeIndex + 1 < m_clips.size()) || (m_loop && m_clips.size() > 1);

    if (hasNextClip && timeRemainingInClip < kTransitionDuration) {
        size_t nextIndex = (activeIndex + 1 < m_clips.size()) ? (activeIndex + 1) : 0;
        const MotionClip& nextClip = m_clips[nextIndex];
        float nextPhase = 2.0f * kPi * nextClip.frequency * t;

        float blendWeight = smoothstep(currentClip.duration - kTransitionDuration,
                                       currentClip.duration, localT);
        gaitFrame = ProceduralGait::evaluateBlend(currentClip.gait, nextClip.gait,
                                                 phase, nextPhase, blendWeight,
                                                 currentClip.params, nextClip.params);
    } else {
        gaitFrame = ProceduralGait::evaluate(currentClip.gait, phase, currentClip.params);
    }

    // Solve spatial position & tangent yaw
    TrajectoryPoint pt = TrajectorySolver::evaluate(m_pathConfig, distance);
    pt.position.y += gaitFrame.verticalBounce;

    frame.worldPosition = pt.position;
    frame.yawDegrees = pt.yawDegrees;
    frame.boneRotations = std::move(gaitFrame.boneRotations);
    frame.valid = true;
    return frame;
}

} // namespace pose
