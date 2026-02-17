#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <unordered_set>

using namespace geode::prelude;
// this is where we declare the original PlayLayer as our own
class $modify(CustomJumpPlayLayer, PlayLayer) {
  struct Fields {
    std::unordered_set<int> heldJumpKeys;
    bool appliedJumpDown = false;
  };

  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
      return false;
    }

    this->addEventListener(
        "jump-key-listener", KeyboardInputEvent(),
        [this](KeyboardInputData &data) {
          auto const binds =
              Mod::get()->getSettingValue<std::vector<Keybind>>("jump-key");
          if (binds.empty()) {
            m_fields->heldJumpKeys.clear();
            return ListenerResult::Propagate;
          }

          Keybind pressed(data.key, data.modifiers);
          bool matchesPress =
              data.action != KeyboardInputData::Action::Release &&
              std::ranges::contains(binds, pressed);
          bool matchesRelease =
              data.action == KeyboardInputData::Action::Release &&
              std::ranges::any_of(binds, [&data](Keybind const &bind) {
                return bind.key == data.key;
              });

          if (data.action == KeyboardInputData::Action::Repeat) {
            return ListenerResult::Propagate;
          }

          if (!matchesPress && !matchesRelease) {
            return ListenerResult::Propagate;
          }

          if (matchesPress) {
            m_fields->heldJumpKeys.insert(static_cast<int>(data.key));
          }
          if (matchesRelease) {
            m_fields->heldJumpKeys.erase(static_cast<int>(data.key));
          }

          return ListenerResult::Propagate;
        });

    return true;
  }

  void update(float dt) {
    PlayLayer::update(dt);
    this->processCustomJumpState();
  }

  void postUpdate(float dt) {
    PlayLayer::postUpdate(dt);
    this->processCustomJumpState();
  }

  bool isCurrentPlayLayer() {
    auto scene = CCScene::get();
    if (!scene) {
      return false;
    }

    auto scenePlayLayer = scene->getChildByType<PlayLayer>(0);
    return scenePlayLayer == this && PlayLayer::get() == this;
  }

  bool isCustomJumpContextValid() {
    if (!this->isCurrentPlayLayer()) {
      return false;
    }
    if (this->m_isPaused || !this->isGameplayActive() ||
        this->m_hasCompletedLevel) {
      return false;
    }
    if (!this->m_player1 || this->m_player1->m_isDead) {
      return false;
    }
    return true;
  }

  bool isAnyConfiguredJumpKeyHeld() {
    auto const binds =
        Mod::get()->getSettingValue<std::vector<Keybind>>("jump-key");
    if (binds.empty()) {
      m_fields->heldJumpKeys.clear();
      return false;
    }

    return std::ranges::any_of(binds, [this](Keybind const &bind) {
      return m_fields->heldJumpKeys.contains(static_cast<int>(bind.key));
    });
  }

  void processCustomJumpState() {
    bool desiredJumpDown =
        this->isCustomJumpContextValid() && this->isAnyConfiguredJumpKeyHeld();

    if (desiredJumpDown == m_fields->appliedJumpDown) {
      return;
    }

    m_fields->appliedJumpDown = desiredJumpDown;
    this->queueJumpButton(m_fields->appliedJumpDown);
  }

  void queueJumpButton(bool down) {
    this->queueButton(1, down, false, geode::utils::getInputTimestamp());
  }
};
