#include <Geode/Geode.hpp>

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/RewardUnlockLayer.hpp>

#include "ui/RollingLayer.hpp"

using namespace geode::prelude;

/*class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

		// only for testing
		GameManager::sharedState()->setUGV("5", true); // unlocked basement
		GameStatsManager::sharedState()->setStat("21", 1000); // demon keys
		GameStatsManager::sharedState()->setStat("43", 1000); // gold keys

		return true;
	}
};*/

class $modify(MyUnlockLayer, RewardUnlockLayer) {
	struct Fields {
		RollingLayer* m_rollingLayer = nullptr;
	};

	bool init(int p0, RewardsPage* p1) {
		if(!RewardUnlockLayer::init(p0, p1)) return false;

		// verify if its secret chests
		if(m_chestType <= 2) return true;

		m_fields->m_rollingLayer = RollingLayer::create(this);
		this->addChild(m_fields->m_rollingLayer, 100);

		return true;
	}

	void startRolling() {
		m_chestSprite->switchToState(ChestSpriteState::Opened, false);

		if(m_fields->m_rollingLayer) {
			m_fields->m_rollingLayer->begin();
		}
	}

	void step2() {
		RewardUnlockLayer::step2();

		// verify if its secret chests
		if(m_chestType <= 2) return;

		// replace step3
		this->stopAllActions();

		auto seq = CCSequence::create(
			CCDelayTime::create(0.3f),
			CCCallFunc::create(this, callfunc_selector(MyUnlockLayer::startRolling)),
			nullptr
		);

		this->runAction(seq);
	}
};