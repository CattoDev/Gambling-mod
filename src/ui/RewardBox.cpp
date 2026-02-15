#include "RewardBox.hpp"

using namespace geode::prelude;

bool RewardBox::init() {
    if(!CCNode::init()) return false;

    this->setContentSize(CCSize { REWARDBOX_WIDTH, REWARDBOX_HEIGHT });

    // background
    m_background = CCLayerColor::create(ccColor4B { 100, 100, 100, 255 });
    m_background->setContentSize(this->getContentSize());
    m_background->setPosition(-m_background->getContentSize() / 2);
    m_background->setOpacity(230);

    this->addChild(m_background);
    
    // bottom glow
    m_bgGlow = CCSprite::createWithSpriteFrameName("d_gradient_01_001.png");

    m_bgGlow->setScale(this->getContentWidth() / m_bgGlow->getContentWidth());
    m_bgGlow->setOpacity(120);

    this->addChild(m_bgGlow, 4);

    // bottom line
    m_drawNode = CCDrawNode::create();
    this->addChild(m_drawNode, 2);

    return true;
}

void RewardBox::setRarityColor(cocos2d::ccColor3B color) {
    m_drawNode->clear();

    auto col = ccc4FFromccc3B(color);
    auto size = this->getContentSize();

    CCPoint points[4] = {
        { -size.width / 2, -size.height / 2 },
        { size.width / 2, -size.height / 2 },
        { size.width / 2, -size.height / 2 + 3.f },
        { -size.width / 2, -size.height / 2 + 3.f }
    };
    m_drawNode->drawPolygon(points, 4, col, 0.f, col);
    m_bgGlow->setColor(color);
}

void RewardBox::setItemsMenu(cocos2d::CCMenu* menu) {
    m_itemsMenu = menu;
    this->addChild(menu, 10);
}

void RewardBox::setHeight(float height) {
    this->setContentHeight(height);

    // resize bg
    m_background->setContentSize(this->getContentSize());
    m_background->setPositionY(-m_background->getContentHeight() / 2);

    // resize glow
    float newGlowHeight = m_bgGlow->getScaledContentHeight();
    m_bgGlow->setPositionY(-this->getContentHeight() / 2.f + newGlowHeight / 2.f);
}

RewardBox* RewardBox::create() {
    auto ret = new (std::nothrow) RewardBox();

    if(ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}