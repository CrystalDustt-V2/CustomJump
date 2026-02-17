#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <unordered_set>

using namespace geode::prelude;

class $modify(CustomJumpPlayLayer, PlayLayer) {
  static constexpr char const* kJumpKeyP1Setting = "jump-key";
  static constexpr char const* kJumpKeyP2Setting = "jump-key-p2";
  static constexpr char const* kEnableJumpKeyP2Setting = "enable-jump-key-p2";

  struct Fields {
    std::unordered_set<int> heldJumpKeysP1;
    std::unordered_set<int> heldJumpKeysP2;
    bool appliedJumpDownP1 = false;
    bool appliedJumpDownP2 = false;
  };

  bool init(GJGameLevel *level, bool useReplay, bool dontCreateObjects) {
    if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
      return false;
    }

    this->addEventListener(
        "jump-key-listener", KeyboardInputEvent(),
        [this](KeyboardInputData &data) {
          if (data.action == KeyboardInputData::Action::Repeat) {
            return ListenerResult::Propagate;
          }

          bool p2Enabled = this->isPlayer2CustomEnabled();
          auto const bindsP1 =
              Mod::get()->getSettingValue<std::vector<Keybind>>(kJumpKeyP1Setting);
          auto const bindsP2 = p2Enabled
                                   ? Mod::get()->getSettingValue<std::vector<Keybind>>(kJumpKeyP2Setting)
                                   : std::vector<Keybind>{};

          if (bindsP1.empty()) {
            m_fields->heldJumpKeysP1.clear();
          }
          if (!p2Enabled || bindsP2.empty()) {
            m_fields->heldJumpKeysP2.clear();
          }

          bool matchedAny = false;
          this->updateHeldKeysForEvent(data, bindsP1, m_fields->heldJumpKeysP1, matchedAny);
          this->updateHeldKeysForEvent(data, bindsP2, m_fields->heldJumpKeysP2, matchedAny);

          if (!matchedAny) {
            return ListenerResult::Propagate;
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

  bool isCustomJumpContextValid(PlayerObject *player) {
    if (!this->isCurrentPlayLayer()) {
      return false;
    }
    if (this->m_isPaused || !this->isGameplayActive() ||
        this->m_hasCompletedLevel) {
      return false;
    }
    if (!player || player->m_isDead) {
      return false;
    }
    return true;
  }

  bool isPlayer2CustomEnabled() {
    auto mod = Mod::get();
    if (!mod->hasSetting(kEnableJumpKeyP2Setting)) {
      return true;
    }
    return mod->getSettingValue<bool>(kEnableJumpKeyP2Setting);
  }

  void updateHeldKeysForEvent(KeyboardInputData const &data,
                              std::vector<Keybind> const &binds,
                              std::unordered_set<int> &heldKeys,
                              bool &matchedAny) {
    if (binds.empty()) {
      return;
    }

    Keybind pressed(data.key, data.modifiers);
    bool matchesPress = data.action != KeyboardInputData::Action::Release &&
                        std::ranges::contains(binds, pressed);
    bool matchesRelease = data.action == KeyboardInputData::Action::Release &&
                          std::ranges::any_of(binds, [&data](Keybind const &bind) {
                            return bind.key == data.key;
                          });

    if (matchesPress) {
      heldKeys.insert(static_cast<int>(data.key));
      matchedAny = true;
    }

    if (matchesRelease) {
      heldKeys.erase(static_cast<int>(data.key));
      matchedAny = true;
    }
  }

  bool isAnyConfiguredJumpKeyHeld(std::vector<Keybind> const &binds,
                                  std::unordered_set<int> const &heldKeys) {
    return std::ranges::any_of(binds, [&heldKeys](Keybind const &bind) {
      return heldKeys.contains(static_cast<int>(bind.key));
    });
  }

  void processCustomJumpState() {
    bool p2Enabled = this->isPlayer2CustomEnabled();
    auto const bindsP1 =
        Mod::get()->getSettingValue<std::vector<Keybind>>(kJumpKeyP1Setting);
    auto const bindsP2 = p2Enabled
                             ? Mod::get()->getSettingValue<std::vector<Keybind>>(kJumpKeyP2Setting)
                             : std::vector<Keybind>{};

    if (bindsP1.empty()) {
      m_fields->heldJumpKeysP1.clear();
    }
    if (!p2Enabled || bindsP2.empty()) {
      m_fields->heldJumpKeysP2.clear();
    }

    bool desiredJumpDownP1 =
        this->isCustomJumpContextValid(this->m_player1) &&
        this->isAnyConfiguredJumpKeyHeld(bindsP1, m_fields->heldJumpKeysP1);
    bool desiredJumpDownP2 =
        p2Enabled && this->isCustomJumpContextValid(this->m_player2) &&
        this->isAnyConfiguredJumpKeyHeld(bindsP2, m_fields->heldJumpKeysP2);

    if (desiredJumpDownP1 != m_fields->appliedJumpDownP1) {
      m_fields->appliedJumpDownP1 = desiredJumpDownP1;
      this->queueJumpButton(m_fields->appliedJumpDownP1, false);
    }

    if (desiredJumpDownP2 != m_fields->appliedJumpDownP2) {
      m_fields->appliedJumpDownP2 = desiredJumpDownP2;
      this->queueJumpButton(m_fields->appliedJumpDownP2, true);
    }
  }

  void queueJumpButton(bool down, bool isPlayer2) {
    this->queueButton(1, down, isPlayer2, geode::utils::getInputTimestamp());
  }
};
