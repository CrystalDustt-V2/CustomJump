#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/Keyboard.hpp>

using namespace geode::prelude;
// bro was here
class $modify(CustomJumpPlayLayer, PlayLayer) {
	struct Fields {
		bool desiredJumpDown = false;
		bool appliedJumpDown = false;
	};

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
			return false;
		}

		this->addEventListener(
			"jump-key-listener",
			KeyboardInputEvent(),
			[this](KeyboardInputData& data) {
				auto const binds = Mod::get()->getSettingValue<std::vector<Keybind>>("jump-key");
				if (binds.empty()) {
					return ListenerResult::Propagate;
				}

				Keybind pressed(data.key, data.modifiers);
				bool matchesPress = std::ranges::contains(binds, pressed);
				bool matchesRelease = data.action == KeyboardInputData::Action::Release &&
					std::ranges::any_of(binds, [&data](Keybind const& bind) {
						return bind.key == data.key;
					});

				if (!matchesPress && !matchesRelease) {
					return ListenerResult::Propagate;
				}

				if (data.action == KeyboardInputData::Action::Repeat) {
					return ListenerResult::Propagate;
				}

				if (!this->isCustomJumpContextValid()) {
					m_fields->desiredJumpDown = false;
					return ListenerResult::Propagate;
				}

				m_fields->desiredJumpDown = data.action != KeyboardInputData::Action::Release;
				return ListenerResult::Propagate;
			}
		);

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
		if (this->m_isPaused || !this->isGameplayActive() || this->m_hasCompletedLevel) {
			return false;
		}
		if (!this->m_player1 || this->m_player1->m_isDead) {
			return false;
		}
		return true;
	}

	void processCustomJumpState() {
		if (!this->isCustomJumpContextValid()) {
			m_fields->desiredJumpDown = false;
		}

		if (m_fields->desiredJumpDown == m_fields->appliedJumpDown) {
			return;
		}

		m_fields->appliedJumpDown = m_fields->desiredJumpDown;
		this->queueJumpButton(m_fields->appliedJumpDown);
	}

	void queueJumpButton(bool down) {
		this->queueButton(1, down, false, geode::utils::getInputTimestamp());
	}
};
