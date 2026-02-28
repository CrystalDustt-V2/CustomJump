#include <Geode/Geode.hpp>
#include <Geode/binding/EditorPauseLayer.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <optional>
#include <vector>

using namespace geode::prelude;

namespace customjump {
static constexpr char const* kJumpKeyP1Setting = "jump-key";
static constexpr char const* kJumpKeyP2Setting = "jump-key-p2";
static constexpr char const* kEnableJumpKeyP2Setting = "enable-jump-key-p2";
static constexpr char const* kStopPropagationSetting = "stop-keybind-propagation";
static constexpr int kKeybindListenerPriority = 0;

bool isPlayer2CustomEnabled() {
  auto mod = Mod::get();
  if (!mod->hasSetting(kEnableJumpKeyP2Setting)) {
    return true;
  }
  return mod->getSettingValue<bool>(kEnableJumpKeyP2Setting);
}

std::vector<Keybind> getJumpBindsP1() {
  auto mod = Mod::get();
  if (!mod->hasSetting(kJumpKeyP1Setting)) {
    return {};
  }
  return mod->getSettingValue<std::vector<Keybind>>(kJumpKeyP1Setting);
}

std::vector<Keybind> getJumpBindsP2(bool p2Enabled) {
  auto mod = Mod::get();
  if (!p2Enabled || !mod->hasSetting(kJumpKeyP2Setting)) {
    return {};
  }
  return mod->getSettingValue<std::vector<Keybind>>(kJumpKeyP2Setting);
}

bool shouldStopKeybindPropagation() {
  auto mod = Mod::get();
  if (!mod->hasSetting(kStopPropagationSetting)) {
    return false;
  }
  return mod->getSettingValue<bool>(kStopPropagationSetting);
}

bool keybindListenerResult() {
  return shouldStopKeybindPropagation() ? ListenerResult::Stop
                                        : ListenerResult::Propagate;
}

bool addHeldBind(std::vector<Keybind>& heldBinds, Keybind const& keybind) {
  if (std::ranges::contains(heldBinds, keybind)) {
    return false;
  }
  heldBinds.push_back(keybind);
  return true;
}

bool removeHeldBindsByKey(std::vector<Keybind>& heldBinds,
                          cocos2d::enumKeyCodes key) {
  auto const before = heldBinds.size();
  std::erase_if(heldBinds, [key](Keybind const& held) { return held.key == key; });
  return heldBinds.size() != before;
}

bool updateHeldBindsForKeybindEvent(Keybind const& keybind, bool down, bool repeat,
                                    std::vector<Keybind> const& configuredBinds,
                                    std::vector<Keybind>& heldBinds) {
  if (configuredBinds.empty() || repeat) {
    return false;
  }
  if (!down) {
    return removeHeldBindsByKey(heldBinds, keybind.key);
  }
  if (!std::ranges::contains(configuredBinds, keybind)) {
    return false;
  }
  return addHeldBind(heldBinds, keybind);
}

bool keybindMatchesRawInput(Keybind const& configuredBind, cocos2d::enumKeyCodes key,
                            KeyboardModifier inputModifiers) {
  if (configuredBind.key != key) {
    return false;
  }
  if (configuredBind.modifiers == KeyboardModifier::None) {
    return true;
  }
  auto const requiredMods = configuredBind.modifiers.value;
  auto const activeMods = inputModifiers.value;
  return (activeMods & requiredMods) == requiredMods;
}

bool updateHeldBindsForRawInput(cocos2d::enumKeyCodes key,
                                KeyboardModifier inputModifiers, bool down,
                                bool repeat,
                                std::vector<Keybind> const& configuredBinds,
                                std::vector<Keybind>& heldBinds) {
  if (configuredBinds.empty()) {
    return false;
  }
  if (repeat) {
    return false;
  }
  if (!down) {
    return removeHeldBindsByKey(heldBinds, key);
  }

  bool changed = false;
  for (auto const& configuredBind : configuredBinds) {
    if (keybindMatchesRawInput(configuredBind, key, inputModifiers)) {
      changed |= addHeldBind(heldBinds, configuredBind);
    }
  }
  return changed;
}

void pruneHeldBindsToConfigured(std::vector<Keybind>& heldBinds,
                                std::vector<Keybind> const& configuredBinds) {
  std::erase_if(heldBinds, [&configuredBinds](Keybind const& held) {
    return !std::ranges::contains(configuredBinds, held);
  });
}

double resolveInputTimestamp(std::optional<double> timestamp) {
  return timestamp.value_or(geode::utils::getInputTimestamp());
}

bool hasAnyHeldBind(std::vector<Keybind> const& heldBinds) {
  return !heldBinds.empty();
}
} // namespace customjump

class $modify(CustomJumpPlayLayer, PlayLayer) {
  struct Fields {
    std::vector<Keybind> heldJumpBindsP1;
    std::vector<Keybind> heldJumpBindsP2;
    bool appliedJumpDownP1 = false;
    bool appliedJumpDownP2 = false;
  };

  bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
      return false;
    }

    // [Geode v5.0.0 Stable Migration]
    this->addEventListener(
        "customjump-p1-play",
        KeybindSettingPressedEventV3(Mod::get(), customjump::kJumpKeyP1Setting),
        [this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
          auto const bindsP1 = customjump::getJumpBindsP1();
          if (bindsP1.empty()) {
            m_fields->heldJumpBindsP1.clear();
            return ListenerResult::Propagate;
          }

          bool matched = customjump::updateHeldBindsForKeybindEvent(
              keybind, down, repeat, bindsP1, m_fields->heldJumpBindsP1);
          if (!matched) {
            return ListenerResult::Propagate;
          }

          this->processCustomJumpState(timestamp);
          return customjump::keybindListenerResult();
        },
        customjump::kKeybindListenerPriority);

    // [Geode v5.0.0 Stable Migration]
    this->addEventListener(
        "customjump-p2-play",
        KeybindSettingPressedEventV3(Mod::get(), customjump::kJumpKeyP2Setting),
        [this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
          if (!customjump::isPlayer2CustomEnabled()) {
            m_fields->heldJumpBindsP2.clear();
            return ListenerResult::Propagate;
          }

          auto const bindsP2 = customjump::getJumpBindsP2(true);
          if (bindsP2.empty()) {
            m_fields->heldJumpBindsP2.clear();
            return ListenerResult::Propagate;
          }

          bool matched = customjump::updateHeldBindsForKeybindEvent(
              keybind, down, repeat, bindsP2, m_fields->heldJumpBindsP2);
          if (!matched) {
            return ListenerResult::Propagate;
          }

          this->processCustomJumpState(timestamp);
          return customjump::keybindListenerResult();
        },
        customjump::kKeybindListenerPriority);

    this->addEventListener(
        "customjump-raw-key-release-play",
        KeyboardInputEvent(),
        [this](KeyboardInputData& data) {
          bool down = data.action != KeyboardInputData::Action::Release;
          bool repeat = data.action == KeyboardInputData::Action::Repeat;

          bool changed = false;
          auto const bindsP1 = customjump::getJumpBindsP1();
          changed |= customjump::updateHeldBindsForRawInput(
              data.key, data.modifiers, down, repeat, bindsP1,
              m_fields->heldJumpBindsP1);

          bool p2Enabled = customjump::isPlayer2CustomEnabled();
          auto const bindsP2 = customjump::getJumpBindsP2(p2Enabled);
          if (p2Enabled) {
            changed |= customjump::updateHeldBindsForRawInput(
                data.key, data.modifiers, down, repeat, bindsP2,
                m_fields->heldJumpBindsP2);
          }

          if (changed) {
            this->processCustomJumpState(data.timestamp);
          }
          return ListenerResult::Propagate;
        },
        customjump::kKeybindListenerPriority);

    this->addEventListener(
        "customjump-raw-mouse-release-play",
        MouseInputEvent(),
        [this](MouseInputData& data) {
          auto const key = MouseInputData::buttonToKeyCode(data.button);
          if (key == cocos2d::enumKeyCodes::KEY_None) {
            return ListenerResult::Propagate;
          }
          bool down = data.action == MouseInputData::Action::Press;

          bool changed = false;
          auto const bindsP1 = customjump::getJumpBindsP1();
          changed |= customjump::updateHeldBindsForRawInput(
              key, data.modifiers, down, false, bindsP1, m_fields->heldJumpBindsP1);

          bool p2Enabled = customjump::isPlayer2CustomEnabled();
          auto const bindsP2 = customjump::getJumpBindsP2(p2Enabled);
          if (p2Enabled) {
            changed |= customjump::updateHeldBindsForRawInput(
                key, data.modifiers, down, false, bindsP2,
                m_fields->heldJumpBindsP2);
          }

          if (changed) {
            this->processCustomJumpState(data.timestamp);
          }
          return ListenerResult::Propagate;
        },
        customjump::kKeybindListenerPriority);

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

  bool isCustomJumpContextValid(PlayerObject* player) {
    if (this->m_isPaused || !this->isGameplayActive() ||
        this->m_hasCompletedLevel) {
      return false;
    }
    if (!player || player->m_isDead) {
      return false;
    }
    return true;
  }

  void processCustomJumpState(std::optional<double> inputTimestamp = std::nullopt) {
    bool p2Enabled = customjump::isPlayer2CustomEnabled();
    auto const bindsP1 = customjump::getJumpBindsP1();
    auto const bindsP2 = customjump::getJumpBindsP2(p2Enabled);

    if (bindsP1.empty()) {
      m_fields->heldJumpBindsP1.clear();
    } else {
      customjump::pruneHeldBindsToConfigured(m_fields->heldJumpBindsP1, bindsP1);
    }
    if (!p2Enabled || bindsP2.empty()) {
      m_fields->heldJumpBindsP2.clear();
    } else {
      customjump::pruneHeldBindsToConfigured(m_fields->heldJumpBindsP2, bindsP2);
    }

    bool desiredJumpDownP1 =
        this->isCustomJumpContextValid(this->m_player1) &&
        customjump::hasAnyHeldBind(m_fields->heldJumpBindsP1);
    bool desiredJumpDownP2 =
        p2Enabled && this->isCustomJumpContextValid(this->m_player2) &&
        customjump::hasAnyHeldBind(m_fields->heldJumpBindsP2);

    if (desiredJumpDownP1 != m_fields->appliedJumpDownP1) {
      m_fields->appliedJumpDownP1 = desiredJumpDownP1;
      this->queueButton(1, m_fields->appliedJumpDownP1, false,
                        customjump::resolveInputTimestamp(inputTimestamp));
    }

    if (desiredJumpDownP2 != m_fields->appliedJumpDownP2) {
      m_fields->appliedJumpDownP2 = desiredJumpDownP2;
      this->queueButton(1, m_fields->appliedJumpDownP2, true,
                        customjump::resolveInputTimestamp(inputTimestamp));
    }
  }
};

class $modify(CustomJumpLevelEditorLayer, LevelEditorLayer) {
  struct Fields {
    std::vector<Keybind> heldJumpBindsP1;
    std::vector<Keybind> heldJumpBindsP2;
    bool appliedJumpDownP1 = false;
    bool appliedJumpDownP2 = false;
    bool playtestActive = false;
  };

  bool init(GJGameLevel* level, bool noUI) {
    if (!LevelEditorLayer::init(level, noUI)) {
      return false;
    }

    // [Geode v5.0.0 Stable Migration]
    this->addEventListener(
        "customjump-p1-editor",
        KeybindSettingPressedEventV3(Mod::get(), customjump::kJumpKeyP1Setting),
        [this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
          auto const bindsP1 = customjump::getJumpBindsP1();
          if (bindsP1.empty()) {
            m_fields->heldJumpBindsP1.clear();
            return ListenerResult::Propagate;
          }

          bool matched = customjump::updateHeldBindsForKeybindEvent(
              keybind, down, repeat, bindsP1, m_fields->heldJumpBindsP1);
          if (!matched) {
            return ListenerResult::Propagate;
          }

          this->processCustomJumpState(timestamp);
          return customjump::keybindListenerResult();
        },
        customjump::kKeybindListenerPriority);

    // [Geode v5.0.0 Stable Migration]
    this->addEventListener(
        "customjump-p2-editor",
        KeybindSettingPressedEventV3(Mod::get(), customjump::kJumpKeyP2Setting),
        [this](Keybind const& keybind, bool down, bool repeat, double timestamp) {
          if (!customjump::isPlayer2CustomEnabled()) {
            m_fields->heldJumpBindsP2.clear();
            return ListenerResult::Propagate;
          }

          auto const bindsP2 = customjump::getJumpBindsP2(true);
          if (bindsP2.empty()) {
            m_fields->heldJumpBindsP2.clear();
            return ListenerResult::Propagate;
          }

          bool matched = customjump::updateHeldBindsForKeybindEvent(
              keybind, down, repeat, bindsP2, m_fields->heldJumpBindsP2);
          if (!matched) {
            return ListenerResult::Propagate;
          }

          this->processCustomJumpState(timestamp);
          return customjump::keybindListenerResult();
        },
        customjump::kKeybindListenerPriority);

    this->addEventListener(
        "customjump-raw-key-release-editor",
        KeyboardInputEvent(),
        [this](KeyboardInputData& data) {
          bool down = data.action != KeyboardInputData::Action::Release;
          bool repeat = data.action == KeyboardInputData::Action::Repeat;

          bool changed = false;
          auto const bindsP1 = customjump::getJumpBindsP1();
          changed |= customjump::updateHeldBindsForRawInput(
              data.key, data.modifiers, down, repeat, bindsP1,
              m_fields->heldJumpBindsP1);

          bool p2Enabled = customjump::isPlayer2CustomEnabled();
          auto const bindsP2 = customjump::getJumpBindsP2(p2Enabled);
          if (p2Enabled) {
            changed |= customjump::updateHeldBindsForRawInput(
                data.key, data.modifiers, down, repeat, bindsP2,
                m_fields->heldJumpBindsP2);
          }

          if (changed) {
            this->processCustomJumpState(data.timestamp);
          }
          return ListenerResult::Propagate;
        },
        customjump::kKeybindListenerPriority);

    this->addEventListener(
        "customjump-raw-mouse-release-editor",
        MouseInputEvent(),
        [this](MouseInputData& data) {
          auto const key = MouseInputData::buttonToKeyCode(data.button);
          if (key == cocos2d::enumKeyCodes::KEY_None) {
            return ListenerResult::Propagate;
          }
          bool down = data.action == MouseInputData::Action::Press;

          bool changed = false;
          auto const bindsP1 = customjump::getJumpBindsP1();
          changed |= customjump::updateHeldBindsForRawInput(
              key, data.modifiers, down, false, bindsP1, m_fields->heldJumpBindsP1);

          bool p2Enabled = customjump::isPlayer2CustomEnabled();
          auto const bindsP2 = customjump::getJumpBindsP2(p2Enabled);
          if (p2Enabled) {
            changed |= customjump::updateHeldBindsForRawInput(
                key, data.modifiers, down, false, bindsP2,
                m_fields->heldJumpBindsP2);
          }

          if (changed) {
            this->processCustomJumpState(data.timestamp);
          }
          return ListenerResult::Propagate;
        },
        customjump::kKeybindListenerPriority);

    return true;
  }

  void update(float dt) {
    LevelEditorLayer::update(dt);
    this->processCustomJumpState();
  }

  void postUpdate(float dt) {
    LevelEditorLayer::postUpdate(dt);
    this->processCustomJumpState();
  }

  void onPlaytest() {
    m_fields->playtestActive = true;
    LevelEditorLayer::onPlaytest();
  }

  void onStopPlaytest() {
    m_fields->playtestActive = false;
    LevelEditorLayer::onStopPlaytest();
  }

  bool isEditorPlaytestPaused() {
    auto scene = CCScene::get();
    if (!scene) {
      return false;
    }
    auto pauseLayer = scene->getChildByType<EditorPauseLayer>(0);
    return pauseLayer && pauseLayer->isVisible();
  }

  bool isCustomJumpContextValid(PlayerObject* player) {
    bool const isPlaytestActive = m_fields->playtestActive || this->m_playbackActive;
    if (!isPlaytestActive || this->isEditorPlaytestPaused()) {
      return false;
    }
    if (!player || player->m_isDead) {
      return false;
    }
    return true;
  }

  void processCustomJumpState(std::optional<double> inputTimestamp = std::nullopt) {
    bool p2Enabled = customjump::isPlayer2CustomEnabled();
    auto const bindsP1 = customjump::getJumpBindsP1();
    auto const bindsP2 = customjump::getJumpBindsP2(p2Enabled);
    bool const isPlaytestActive = m_fields->playtestActive || this->m_playbackActive;

    if (!isPlaytestActive) {
      m_fields->heldJumpBindsP1.clear();
      m_fields->heldJumpBindsP2.clear();
    }
    if (bindsP1.empty()) {
      m_fields->heldJumpBindsP1.clear();
    } else {
      customjump::pruneHeldBindsToConfigured(m_fields->heldJumpBindsP1, bindsP1);
    }
    if (!p2Enabled || bindsP2.empty()) {
      m_fields->heldJumpBindsP2.clear();
    } else {
      customjump::pruneHeldBindsToConfigured(m_fields->heldJumpBindsP2, bindsP2);
    }

    bool desiredJumpDownP1 =
        this->isCustomJumpContextValid(this->m_player1) &&
        customjump::hasAnyHeldBind(m_fields->heldJumpBindsP1);
    bool desiredJumpDownP2 =
        p2Enabled && this->isCustomJumpContextValid(this->m_player2) &&
        customjump::hasAnyHeldBind(m_fields->heldJumpBindsP2);

    if (desiredJumpDownP1 != m_fields->appliedJumpDownP1) {
      m_fields->appliedJumpDownP1 = desiredJumpDownP1;
      this->queueButton(1, m_fields->appliedJumpDownP1, false,
                        customjump::resolveInputTimestamp(inputTimestamp));
    }

    if (desiredJumpDownP2 != m_fields->appliedJumpDownP2) {
      m_fields->appliedJumpDownP2 = desiredJumpDownP2;
      this->queueButton(1, m_fields->appliedJumpDownP2, true,
                        customjump::resolveInputTimestamp(inputTimestamp));
    }
  }
};
