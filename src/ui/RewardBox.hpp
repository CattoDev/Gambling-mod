#pragma once

#include <Geode/Geode.hpp>

#define REWARDBOX_WIDTH 100.f
#define REWARDBOX_HEIGHT 80.f

class RewardBox : public cocos2d::CCNode {
public:
    cocos2d::CCLayerColor* m_background;
    cocos2d::CCDrawNode* m_drawNode;
    cocos2d::CCSprite* m_bgGlow;
    cocos2d::CCMenu* m_itemsMenu = nullptr;

public:
    bool init();
    void setRarityColor(cocos2d::ccColor3B color);
    void setItemsMenu(cocos2d::CCMenu* menu);
    void setHeight(float height);

    static RewardBox* create();
};