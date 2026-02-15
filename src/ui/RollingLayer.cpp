#include "RollingLayer.hpp"
#include <random>
#include <Geode/utils/Keyboard.hpp>

using namespace geode::prelude;

#define BOXCOUNT 40
#define PADDING 10

int g_currencySpriteTypes[] = { 4, 5, 6, 7, 8, 9, 0, 3, 0, 10, 11, 12, 13, 14, 16 };

bool RollingLayer::init(RewardUnlockLayer* unlockLayer) {
    if(!CCLayerColor::initWithColor(ccColor4B { 0, 0, 0, 0 })) return false;

    auto winSize = CCDirector::get()->getWinSize();

    m_unlockLayer = unlockLayer;

    // needed cuz scaling RollingLayer layer causes funky stuff
    m_contentLayer = CCLayer::create();

    this->addChild(m_contentLayer);

    m_itemsMenu = CCMenu::create();
    m_itemsMenu->setPosition(CCPointZero);
    m_itemsMenu->setLayout(RowLayout::create()->setAutoScale(false)->setGap(PADDING)->setAutoGrowAxis(0.f), true, false);

    // extra menu cuz yes
    auto menu = CCMenu::create();

    menu->addChild(m_itemsMenu);
    m_contentLayer->addChild(menu, 5);

    this->createRewardBoxes();
    this->scatterRewards();

    // middle line
    m_lineNode = CCDrawNode::create();
    m_lineNode->setPosition(winSize / 2);
    m_contentLayer->addChild(m_lineNode, 10);

    m_linePos = m_lineNode->getPosition();

    float boxHeightHalf = this->rewardBoxHeight() / 2;

    CCPoint points[4] = {
        CCPoint { 0.f, boxHeightHalf },
        CCPoint { 0.f, -boxHeightHalf },
        CCPoint { 0.f, -boxHeightHalf },
        CCPoint { 0.f, boxHeightHalf }
    };
    m_lineNode->drawPolygon(points, 4, ccColor4F { 1.f, 1.f, 0.f, 1.f }, 0.f, ccColor4F { 1.f, 1.f, 0.f, 1.f });

    // background glow
    m_backgroundGlow = CCNode::create();
    {
        const char* sprName = "d_gradient_c_02_001.png";

        auto spr1 = CCSprite::createWithSpriteFrameName(sprName);
        auto spr2 = CCSprite::createWithSpriteFrameName(sprName);
        auto spr3 = CCSprite::createWithSpriteFrameName(sprName);
        auto spr4 = CCSprite::createWithSpriteFrameName(sprName);

        m_backgroundGlow->addChild(spr1);
        m_backgroundGlow->addChild(spr2);
        m_backgroundGlow->addChild(spr3);
        m_backgroundGlow->addChild(spr4);

        auto size = spr1->getContentSize();

        spr1->setPosition(CCPoint { -size.width / 2, size.height / 2 });

        spr2->setFlipX(true);
        spr2->setPosition(CCPoint { size.width / 2, size.height / 2 });

        spr3->setFlipY(true);
        spr3->setPosition(CCPoint { -size.width / 2, -size.height / 2 });

        spr4->setFlipX(true);
        spr4->setFlipY(true);
        spr4->setPosition(CCPoint { size.width / 2, -size.height / 2 });
    }
    m_backgroundGlow->setScale(12.f);

    // glow clipping node
    auto glowClipStencil = CCSprite::createWithSpriteFrameName("colourPickerBackground.png");
    glowClipStencil->setScale(2.25f);
    auto glowClipNode = CCClippingNode::create(glowClipStencil);
    glowClipNode->setInverted(true);
    glowClipNode->setAlphaThreshold(.2f);
    glowClipNode->setPosition(winSize / 2);

    glowClipNode->addChild(m_backgroundGlow);

    m_contentLayer->addChild(glowClipNode);

    // prepare
    m_contentLayer->setScaleY(0.f);

    m_inputEvent = KeyboardInputEvent(KEY_Escape).listen([this](KeyboardInputData& event) {
        // quick open
        this->animationFinished();

        return ListenerResult::Propagate;
    });
    
    return true;
}

void RollingLayer::begin() {
    this->setKeyboardEnabled(true);

    this->setupTargetBox();
    this->startAnimation();
}

RewardBox* RollingLayer::getTargetBox() {
    return typeinfo_cast<RewardBox*>(m_itemsMenu->getChildren()->objectAtIndex(BOXCOUNT - 4));
}

void RollingLayer::setupTargetBox() {
    if(!m_unlockLayer) return;

    // random chance for gold item
    std::mt19937_64 eng;
    std::random_device rdev;
    eng.seed(rdev());

    std::uniform_int_distribution<int> rDist(1, 400);
    //std::uniform_int_distribution<int> rDist(1, 1);

    int randomNum = rDist(eng);

    log::debug("rolled num: {}", randomNum);

    if(randomNum == 1) { // 1 in 400 (0.26%)
        auto goldItemSpr = CCSprite::create("gamb_goldItem.png"_spr);
        goldItemSpr->setScale(.8f);

        auto box = this->getTargetBox();

        box->setHeight(this->rewardBoxHeight());
        box->setRarityColor(ccColor3B { 255, 255, 0 });
        box->addChild(goldItemSpr, 10);
        
        m_isGold = true;
        return;
    }

    // otherwise do boring item
    auto item = m_unlockLayer->m_rewardItem;

    this->setupRewardBoxWithItem(this->getTargetBox(), item);
}

// CurrencySprite::rewardToSpriteType recreation
CurrencySpriteType RollingLayer::rewardToSpriteType(SpecialRewardItem item) {
    int idx = static_cast<int>(item) - 1;
    return static_cast<CurrencySpriteType>(g_currencySpriteTypes[idx]);
}

cocos2d::ccColor3B RollingLayer::rarityColorForRewardItem(GJRewardItem* item) {
    // get highest rarity reward
    int highestRarity = 0;
    for(auto& obj : CCArrayExt<GJRewardObject*>(item->m_rewardObjects)) {
        // orbs (default)
        int rarity = 0;

        // diamonds
        if(obj->m_specialRewardItem == SpecialRewardItem::Diamonds) {
            rarity = 1;
        }

        // shard
        if(obj->isSpecialType()) {
            rarity = 2;
        }

        // icon
        if(obj->m_specialRewardItem == SpecialRewardItem::CustomItem) {
            rarity = 3;

            // icon + another reward is red rarity
            if(item->m_rewardObjects->count() > 1) rarity = 4;
        }

        // death effect
        if(obj->m_unlockType == UnlockType::Death) {
            rarity = 4;
        }

        if(highestRarity < rarity) highestRarity = rarity;
    }

    // default color (gray)
    ccColor3B color = ccColor3B { 100, 100, 150 };

    switch(highestRarity) {
        // blue
        case 1: {
            color = ccColor3B { 0, 50, 255 };
        } break;

        // purple
        case 2: {
            color = ccColor3B { 150, 0, 255 };
        } break;

        // pink
        case 3: {
            color = ccColor3B { 255, 0, 255 };
        } break;

        // red
        case 4: {
            color = ccColor3B { 255, 0, 0 };
        } break;
        
        default: break;
    }

    // default
    return color;
}

void RollingLayer::createRewardBoxes() {
    for(int i = 0; i < BOXCOUNT; i++) {
        auto box = RewardBox::create();
        box->setTag(i);

        m_itemsMenu->addChild(box);
    }
    m_itemsMenu->updateLayout();

    // reposition menu
    auto box = typeinfo_cast<RewardBox*>(m_itemsMenu->getChildren()->objectAtIndex(0));

    float newX = box->getPositionX();
    m_startX = newX;
    this->moveToRewardX(newX);

    // prepare anim
    auto targetBox = this->getTargetBox();
    float targetX = targetBox->getPositionX();

    m_moveAmountX = targetX - newX;
}

void RollingLayer::scatterRewards() {
    auto gsm = GameStatsManager::sharedState();

    auto rewards = gsm->m_allTreasureRoomChests;

    auto rewardMeta = this->rewardMetaForRewardType(static_cast<GJRewardType>(m_unlockLayer->m_chestType));

    log::debug("m_chestType: {}, ({}, {})", m_unlockLayer->m_chestType, rewardMeta.min, rewardMeta.max);

    std::mt19937_64 eng;
    std::random_device rdev;
    eng.seed(rdev());

    std::uniform_int_distribution<int> rDist(rewardMeta.min, rewardMeta.max);

    for(int i = 0; i < 40; i++) {
        // skip the target box
        if(i == this->getTargetBox()->getTag()) continue;

        int randNum = rDist(eng);

        auto rewardItem = typeinfo_cast<GJRewardItem*>(rewards->objectForKey(randNum));

        auto rewardBox = typeinfo_cast<RewardBox*>(m_itemsMenu->getChildren()->objectAtIndex(i));    
        this->setupRewardBoxWithItem(rewardBox, rewardItem);
    }
}

void RollingLayer::setupRewardBoxWithItem(RewardBox* box, GJRewardItem* item) {
    auto menu = this->createMenuFromRewardObjects(item->m_rewardObjects);

    // get box height
    float boxHeight = this->rewardBoxHeight();

    // add to reward box
    box->setHeight(boxHeight);
    box->setRarityColor(this->rarityColorForRewardItem(item));
    box->setItemsMenu(menu);
}

void RollingLayer::moveToRewardX(float newX) {
    m_itemsMenu->setPositionX(-newX);
}

void RollingLayer::startAnimation() {
    // add a bit of offset cuz funny
    std::mt19937_64 eng;
    std::random_device rdev;
    eng.seed(rdev());

    auto rewardBox = typeinfo_cast<RewardBox*>(m_itemsMenu->getChildren()->objectAtIndex(0));
    float width = rewardBox->getContentWidth();
    std::uniform_real_distribution<float> rDist(5.f, width - 5.f);

    float offset = rDist(eng);

    m_moveAmountX += (-width / 2) + offset;

    // animate background glow (gotta do it separately damn it)
    for(auto spr : CCArrayExt<CCSprite*>(m_backgroundGlow->getChildren())) {
        spr->runAction(CCFadeTo::create(.25f, 40));
    }

    this->scheduleUpdate();
}

void RollingLayer::update(float dt) {
    // animate
    #define MAXOPACITY 105
    #define FADEIN_TIME .25f
    #define SCALE_TIME .25f
    #define TOTAL_TIME 5.f

    // fix the skip in animation from the lag of loading icons
    if(m_smoothFix > 0) {
        dt = CCDirector::get()->getAnimationInterval();
        m_smoothFix--;
    }

    m_timeElapsed += dt;

    // fade
    if(m_timeElapsed < FADEIN_TIME) {
        this->setOpacity(static_cast<GLubyte>(MAXOPACITY * m_timeElapsed / FADEIN_TIME));
    }
    else {
        this->setOpacity(MAXOPACITY);
    }

    // scale
    if(m_timeElapsed < SCALE_TIME) {
        m_contentLayer->setScaleY(m_timeElapsed / SCALE_TIME);
    }
    else {
        m_contentLayer->setScaleY(1.f);
    }

    // move
    float x = m_timeElapsed / TOTAL_TIME;
    //float posFactor = 1.f + std::powf(x - 1, 3.f); // old
    float posFactor = -std::powf(x, 2.f) + 2 * x;

    float newX = m_moveAmountX * posFactor;

    this->moveToRewardX(m_startX + newX);

    // animation done
    if(m_timeElapsed > TOTAL_TIME) {
        this->animationFinished();

        return;
    }

    // check if a RewardBox collides with yellow line
    for(auto box : CCArrayExt<RewardBox*>(m_itemsMenu->getChildren())) {
        float width = box->getContentWidth();
        auto boxPos = m_itemsMenu->convertToWorldSpace(box->getPosition());

        if(boxPos.x - width / 2 <= m_linePos.x && boxPos.x + width / 2 >= m_linePos.x) {
            if(box != m_hoveringBox) {
                // sound (yes its literally just the opening sound sped up)
                FMODAudioEngine::sharedEngine()->playEffect("chestClick.ogg", 3.f, 5.f, 1.f);

                m_hoveringBox = box;
            } 
        }   
    }
}

/*void RollingLayer::keyDown(enumKeyCodes key) {
    if(key == enumKeyCodes::KEY_Escape) {
        // quick open
        this->animationFinished();
    }
}*/

void RollingLayer::animationFinished() {
    this->unscheduleUpdate();

    if(m_isGold) {
        // epic gold-specific thing
        FMODAudioEngine::sharedEngine()->playEffect("gamb_gold.ogg"_spr, 1.f, 1.f, 1.f);

        auto winSize = CCDirector::get()->getWinSize();
        auto spr = CCSprite::create("epicpic.png"_spr);
        spr->setPosition(winSize / 2);

        auto sprSize = spr->getContentSize();

        spr->setScaleX(winSize.width / sprSize.width);
        spr->setScaleY(winSize.height / sprSize.height);

        this->addChild(spr, 100);

        this->runAction(
            CCSequence::create(
                CCDelayTime::create(1.f),
                CCCallFunc::create(this, callfunc_selector(RollingLayer::closeAndAward)),
                nullptr
            )
        );
    }
    else {
        this->closeAndAward();
    }
}

RollingLayer::RewardMeta RollingLayer::rewardMetaForRewardType(GJRewardType rewardType) {
    switch(rewardType) {
        case GJRewardType::SmallTreasure: {
            return RewardMeta { 1, 400 };
        } break;

        case GJRewardType::LargeTreasure: {
            return RewardMeta { 1001, 1100 };
        } break;

        case GJRewardType::Key10Treasure: {
            return RewardMeta { 2001, 2060 };
        } break;

        case GJRewardType::Key25Treasure: {
            return RewardMeta { 3001, 3024 };
        } break;

        case GJRewardType::Key50Treasure: {
            return RewardMeta { 4001, 4012 };
        } break;

        case GJRewardType::Key100Treasure: {
            return RewardMeta { 5001, 5008 };
        } break;

        case GJRewardType::Gold: {
            return RewardMeta { 6001, 6020 };
        } break;

        default: break;
    }
    
    return RewardMeta { 0, 0 };
}

float RollingLayer::rewardBoxHeight() {
    switch(static_cast<GJRewardType>(m_unlockLayer->m_chestType)) {
        case GJRewardType::SmallTreasure: {
            return 80.f;
        } break;

        case GJRewardType::LargeTreasure: {
            return 80.f;
        } break;

        case GJRewardType::Key10Treasure: {
            return 100.f;
        } break;

        case GJRewardType::Key25Treasure: {
            return 130.f;
        } break;

        case GJRewardType::Key50Treasure: {
            return 170.f;
        } break;

        case GJRewardType::Key100Treasure: {
            return 200.f;
        } break;

        case GJRewardType::Gold: {
            return 100.f;
        } break;
        
        default: break;
    }
    
    return 0.f;
}

CCMenu* RollingLayer::createMenuFromRewardObjects(CCArray* rewardObjects) {
    auto menu = CCMenu::create();

    for(auto& obj : CCArrayExt<GJRewardObject*>(rewardObjects)) {
        // icons
        if(obj->m_specialRewardItem == SpecialRewardItem::CustomItem) {
            auto icon = GJItemIcon::create(obj->m_unlockType, obj->m_itemID, ccColor3B { 175, 175, 175 }, ccColor3B { 255, 255, 255 }, false, false, true, ccColor3B { 255, 255, 255 });

            menu->addChild(icon);
        }
        // diamonds / shards / orbs
        else {
            auto subMenu = CCMenu::create();

            // label
            auto label = CCLabelBMFont::create(fmt::format("+{}", obj->m_total).c_str(), "bigFont.fnt");
            label->limitLabelWidth(50.f, 1.f, 0.1f);

            subMenu->addChild(label);

            // sprite
            auto sprType = this->rewardToSpriteType(obj->m_specialRewardItem);
            auto spr = CurrencySprite::create(sprType, false);
            spr->setScale(.7f);
            subMenu->addChild(spr);

            subMenu->setLayout(RowLayout::create()->setGap(5.f)->setAutoScale(false));
            subMenu->setPosition(CCPointZero);
            subMenu->setContentHeight(22.f);

            menu->addChild(subMenu);
        }
    }

    menu->setLayout(ColumnLayout::create()->setAxisReverse(true));
    menu->setPosition(CCPointZero);

    return menu;
}

void RollingLayer::closeAndAward() {
    m_unlockLayer->playRewardEffect();

    this->removeFromParent();
}

RollingLayer* RollingLayer::create(RewardUnlockLayer* unlockLayer) {
    auto ret = new (std::nothrow) RollingLayer();

    if(ret && ret->init(unlockLayer)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}