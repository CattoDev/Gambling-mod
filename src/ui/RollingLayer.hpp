#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/RewardUnlockLayer.hpp>

#include "RewardBox.hpp"

class RollingLayer : public cocos2d::CCLayerColor {
public:
    cocos2d::CCLayer* m_contentLayer;
    RewardUnlockLayer* m_unlockLayer = nullptr;
    cocos2d::CCMenu* m_itemsMenu;
    cocos2d::CCDrawNode* m_lineNode;
    cocos2d::CCNode* m_backgroundGlow;
    float m_timeElapsed;
    float m_startX;
    float m_moveAmountX;
    cocos2d::CCPoint m_linePos;
    RewardBox* m_hoveringBox = nullptr;
    bool m_isGold = false;
    int m_smoothFix = 2;

public:
    struct RewardMeta {
        int min;
        int max;
    };

public:
    bool init(RewardUnlockLayer* unlockLayer);
    void begin();
    RewardBox* getTargetBox();
    void setupTargetBox();
    CurrencySpriteType rewardToSpriteType(SpecialRewardItem item);
    cocos2d::ccColor3B rarityColorForRewardItem(GJRewardItem* item);
    void createRewardBoxes();
    void scatterRewards();
    void setupRewardBoxWithItem(RewardBox* box, GJRewardItem* item);
    void moveToRewardX(float newX);
    void startAnimation();
    void animationFinished();
    RewardMeta rewardMetaForRewardType(GJRewardType rewardType);
    float rewardBoxHeight();
    cocos2d::CCMenu* createMenuFromRewardObjects(cocos2d::CCArray* rewardObjects);
    void closeAndAward();

    void update(float dt) override;
    void keyDown(cocos2d::enumKeyCodes) override;

    static RollingLayer* create(RewardUnlockLayer* unlockLayer);
};