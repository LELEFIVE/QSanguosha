#include "new_standard.h"
#include "standard-skillcards.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"

NewLeijiCard::NewLeijiCard(){

}

bool NewLeijiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void NewLeijiCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhangjiao = effect.from;
    ServerPlayer *target = effect.to;

    Room *room = zhangjiao->getRoom();
    room->setEmotion(target, "bad");

    JudgeStruct judge;
    judge.pattern = QRegExp("(.*):(spade|club):(.*)");
    judge.good = false;
    judge.reason = "new_leiji";
    judge.who = target;

    room->judge(judge);

    if(judge.isBad()){
        DamageStruct damage;
        damage.card = NULL;
        damage.from = zhangjiao;
        damage.to = target;
        damage.nature = DamageStruct::Thunder;

        room->damage(damage);
        if(zhangjiao->isWounded()){
            RecoverStruct rec;
            room->recover(zhangjiao, rec);
        }
    }else
        room->setEmotion(zhangjiao, "bad");
}

class NewLeijiViewAsSkill: public ZeroCardViewAsSkill{
public:
    NewLeijiViewAsSkill():ZeroCardViewAsSkill("new_leiji"){

    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@new_leiji";
    }

    virtual const Card *viewAs() const{
        return new NewLeijiCard;
    }
};

class NewLeiji: public TriggerSkill{
public:
    NewLeiji():TriggerSkill("new_leiji"){
        events << CardAsked << CardResponsed;
        view_as_skill = new NewLeijiViewAsSkill;
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *zhangjiao, QVariant &data) const{
        if(event == CardAsked){
            if(data.toString() == "jink")
                zhangjiao->setFlags("new_leiji_invoke");
        }
        else{
            CardStar card_star = data.value<CardStar>();
            if(!card_star->inherits("Jink") || !zhangjiao->hasFlag("new_leiji_invoke"))
                return false;

            zhangjiao->setFlags("-new_leiji_invoke");
            room->askForUseCard(zhangjiao, "@@new_leiji", "@new_leiji");
        }
        return false;
    }
};

class Yaowu:public MasochismSkill{
public:
    Yaowu():MasochismSkill("yaowu"){
    }

    virtual void onDamaged(ServerPlayer *huaxiong, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = huaxiong->getRoom();

        if(from && from->isAlive() && damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()){
            room->playSkillEffect(objectName());
            LogMessage log;
            log.from = huaxiong;
            log.arg = objectName();
            log.type = "#TriggerSkill";
            room->sendLog(log);

            if(from->isWounded())
                room->recover(from, RecoverStruct(), true);
            else
                from->drawCards(1);
        }
    }
};

class Wangzun:public TriggerSkill{
public:
    Wangzun():TriggerSkill("wangzun"){
        events << PhaseChange << Death;
    }

    virtual bool triggerable(const ServerPlayer *pp) const{
        return pp != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *lord, QVariant &data) const{
        if(event == PhaseChange && lord->isLord()){
            if(lord->getPhase() == Player::Start){
                QList<ServerPlayer *> yuanshus = room->findPlayersBySkillName(objectName());
                foreach(ServerPlayer *yuanshu, yuanshus){
                    if(yuanshu->askForSkillInvoke(objectName(), data)){
                        yuanshu->drawCards(1);
                        lord->addMark("wangzun");
                    }
                }
            }
            else if(lord->getPhase() == Player::NotActive)
                lord->loseAllMarks("wangzun");
        }
        else if(event == Death && lord->hasSkill(objectName()))
            lord->loseAllMarks("wangzun");
        return false;
    }
};

class WangzunKeep: public MaxCardsSkill{
public:
    WangzunKeep():MaxCardsSkill("#wangzun"){
    }

    virtual int getExtra(const Player *target) const{
        if(target->hasMark("wangzun"))
            return - target->getMark("wangzun");
        else
            return 0;
    }
};

class Tongji: public ProhibitSkill{
public:
    Tongji():ProhibitSkill("tongji"){
    }

    virtual bool prohibitable(const Player *) const{
        return true;
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        if(card->isKindOf("Slash") && !from->hasSkill(objectName())){
            const Player *yuanshu = NULL;
            QList<const Player *> players = from->getSiblings();
            players << from;
            foreach(const Player *player, players){
                if(player->isAlive() && player->hasSkill(objectName())){
                    yuanshu = player;
                    players.removeOne(player);
                    break;
                }
            }
            if(!yuanshu || yuanshu->getHandcardNum() <= yuanshu->getHp() || !from->inMyAttackRange(yuanshu))
                return false;
            return !to->hasSkill(objectName());
        }
        else
            return false;
    }
};

class NewJizhi:public TriggerSkill{
public:
    NewJizhi():TriggerSkill("new_jizhi"){
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *yueying, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        const Card *card = use.card;
        if(card->isKindOf("TrickCard")){
            if(room->askForSkillInvoke(yueying, objectName())){
                room->playSkillEffect(objectName());
                const Card *peek = room->peek();
                room->showCard(yueying, peek->getId());
                if(!peek->isKindOf("BasicCard"))
                    yueying->drawCards(1);
                else{
                    const Card *my_card = room->askForCard(yueying, ".", "@newjizhi:::" + peek->objectName(), data, CardDiscarded);
                    if(my_card && my_card != card)
                        yueying->obtainCard(peek);
                    else
                        room->throwCard(peek);
                }
            }
        }

        return false;
    }
};

class NewQicai: public TriggerSkill{
public:
    NewQicai():TriggerSkill("new_qicai"){
        events << CardThrow;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        QString datastr = data.toString();
        const Card *card = Sanguosha->getCard(datastr.split(":").first().toInt());
        ServerPlayer *thrower = room->findPlayer(datastr.split(":").last());
        if(!thrower || thrower == player)
            return false;
        if(room->getCardPlace(card->getEffectiveId()) == Player::Equip){
            if(!card->isKindOf("Horse")){
                LogMessage log;
                log.type = "#NewQicai";
                log.from = thrower;
                log.to << player;
                log.arg = objectName();
                room->sendLog(log);
                room->playSkillEffect(objectName());
                return true;
            }
        }

        return false;
    }
};

class NewQicaiD: public TargetModSkill {
public:
    NewQicaiD(): TargetModSkill("#newqicai") {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const{
        if (from->hasSkill("new_qicai"))
            return 998;
        else
            return 0;
    }
};

NewRendeCard::NewRendeCard(){
    will_throw = false;
    target_fixed = true;
    mute = true;
}

void NewRendeCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    QList<int> rende_cards = getSubcards();
    int num = 0;
    while(room->askForYiji(source, rende_cards))
        num++;
    if(num > 0)
        room->playSkillEffect("new_rende");
    if(num >= 2){
        RecoverStruct recover;
        recover.card = this;
        recover.who = source;
        room->recover(source, recover);
    }
}

class NewRende:public ViewAsSkill{
public:
    NewRende():ViewAsSkill("new_rende"){
    }

    virtual bool viewFilter(const QList<CardItem *> &, const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("NewRendeCard");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        NewRendeCard *rende_card = new NewRendeCard;
        rende_card->setSkillName("new_rende");
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class NewLijian: public OneCardViewAsSkill{
public:
    NewLijian():OneCardViewAsSkill("new_lijian"){
    }

    virtual bool viewFilter(const CardItem *) const{
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("LijianCard");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        LijianCard *lijian_card = new LijianCard;
        lijian_card->setSkillName(objectName());
        lijian_card->addSubcard(card_item->getFilteredCard());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 0;
    }
};

class NewJushou: public PhaseChangeSkill{
public:
    NewJushou():PhaseChangeSkill("new_jushou"){
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if(target->getPhase() == Player::Finish){
            Room *room = target->getRoom();
            if(room->askForSkillInvoke(target, objectName())){
                target->drawCards(1);
                target->turnOver();
                room->playSkillEffect(objectName());
            }
        }
        return false;
    }
};

class Jiewei: public TriggerSkill{
public:
    Jiewei():TriggerSkill("jiewei"){
        events << TurnedOver << CardUsed;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(player->hasFlag("JieweiFlag"))
                player->tag["JieWeiCard"] = use.card->getType();
            return false;
        }
        if(player->askForSkillInvoke(objectName())){
            player->drawCards(1);
            player->setFlags("JieweiFlag");
            if(room->askForUseCard(player, "TrickCard,EquipCard", "@jiewei")){
                QString leibie = player->tag["JieWeiCard"].toString();

                QList<ServerPlayer *> victims;
                foreach(ServerPlayer *p, room->getAllPlayers()){
                    if(p->getCards("ej").isEmpty())
                        continue;
                    foreach(const Card *card, p->getCards("ej")){
                        if(card->getType() == leibie){
                            victims << p;
                            break;
                        }
                    }
                }
                if(!victims.isEmpty() && player->askForSkillInvoke(objectName())){
                    ServerPlayer *target = room->askForPlayerChosen(player, victims, objectName());
                    int card_id = room->askForCardChosen(player, target, "ej", objectName());
                    if(Sanguosha->getCard(card_id)->getType() == leibie)
                        room->throwCard(card_id, target, player);
                }
            }
            player->setFlags("-JieweiFlag");
        }
        return false;
    }
};

class NewBuqu: public TriggerSkill{
public:
    NewBuqu():TriggerSkill("new_buqu"){
        events << Dying;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
        LogMessage log;
        log.from = player;
        log.arg = objectName();
        log.type = "#TriggerSkill";
        room->sendLog(log);

        int vul = room->drawCard();
        const Card *new_card = Sanguosha->getCard(vul);
        QList<int> vuls = player->getPile("vulnus");
        player->addToPile("vulnus", vul);
        bool xiangtong = false;
        foreach(int v, vuls){
            const Card *card = Sanguosha->getCard(v);
            if(card->getNumber() == new_card->getNumber()){
                xiangtong = true;
                break;
            }
        }
        if(xiangtong)
            room->throwCard(vul, player);
        else{
            RecoverStruct r;
            r.recover = player->getLostHp() - player->getMaxHp() + 1;
            room->recover(player, r, true);
            room->setPlayerProperty(player, "hp", 1);
        }

        return false;
    }
};

class NewBuqux: public MaxCardsSkill{
public:
    NewBuqux():MaxCardsSkill("#new_buqux"){
    }

    virtual int getExtra(const Player *target) const{
        if(target->hasSkill("new_buqu") && !target->getPile("vulnus").isEmpty())
            return - target->getPile("vulnus").count();
        else
            return 0;
    }
};

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>


class Chanyuan: public TriggerSkill {
public:
    Chanyuan(): TriggerSkill("chanyuan") {
        events << GameStart << HpChanged << MaxHpChanged << SkillAcquire << SkillLose;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == SkillLose) {
            if (data.toString() == objectName()) {
                QStringList chanyuan_skills = player->tag["ChanyuanSkills"].toStringList();
                foreach (QString skill_name, chanyuan_skills)
                    room->setPlayerMark(player, "Qingcheng" + skill_name, 0);
                player->tag["ChanyuanSkills"] = QVariant();
            }
            return false;
        } else if (triggerEvent == SkillAcquire) {
            if (data.toString() != objectName())
                return false;
        }

        if (!TriggerSkill::triggerable(player))
            return false;

        if (player->getHp() == 1) {
            QStringList chanyuan_skills = player->tag["ChanyuanSkills"].toStringList();
            QList<const Skill *> skills = player->getVisibleSkillList();
            foreach (const Skill *skill, skills) {
                if (skill->objectName() != objectName() && skill->getLocation() == Skill::Right
                    && !skill->inherits("SPConvertSkill")
                    && !chanyuan_skills.contains(skill->objectName())) {
                    player->addMark("Qingcheng" + skill->objectName());
                    chanyuan_skills.append(skill->objectName());
                }
            }
            player->tag["ChanyuanSkills"] = QVariant::fromValue(chanyuan_skills);
        } else {
            QStringList chanyuan_skills = player->tag["ChanyuanSkills"].toStringList();
            foreach (QString skill_name, chanyuan_skills)
                room->setPlayerMark(player, "Qingcheng" + skill_name, 0);
            player->tag["ChanyuanSkills"] = QVariant();
        }
        return false;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

NewStandardPackage::NewStandardPackage()
    :Package("new_standard")
{
    General *new_zhangjiao = new General(this, "new_zhangjiao$", "qun", 3);
    new_zhangjiao->addSkill(new NewLeiji);
    new_zhangjiao->addSkill("guidao");
    new_zhangjiao->addSkill("huangtian");
    addMetaObject<NewLeijiCard>();

    General *new_huaxiong = new General(this, "new_huaxiong", "qun", 6);
    new_huaxiong->addSkill(new Yaowu);

    General *new_yuanshu = new General(this, "new_yuanshu", "qun");
    new_yuanshu->addSkill(new Wangzun);
    new_yuanshu->addSkill(new WangzunKeep);
    new_yuanshu->addSkill(new Tongji);

    General *new_huangyueying = new General(this, "new_huangyueying", "shu", 3, false);
    new_huangyueying->addSkill(new NewJizhi);
    new_huangyueying->addSkill(new NewQicai);
    new_huangyueying->addSkill(new NewQicaiD);
    //related_skills.insertMulti("new_qicai", "#newqicai");

    General *new_liubei = new General(this, "new_liubei$", "shu");
    new_liubei->addSkill(new NewRende);
    new_liubei->addSkill("jijiang");
    addMetaObject<NewRendeCard>();

    General *new_diaochan = new General(this, "new_diaochan", "qun", 3, false, true);
    new_diaochan->addSkill(new NewLijian);
    new_diaochan->addSkill("biyue");

    General *new_caoren = new General(this, "new_caoren", "wei");
    new_caoren->addSkill(new NewJushou);
    new_caoren->addSkill(new Jiewei);

    General *new_zhoutai = new General(this, "new_zhoutai", "wu");
    new_zhoutai->addSkill(new NewBuqu);
    skills << new NewBuqux;
/*
    General *new_yuji = new General(this, "new_yuji", "qun", 3);
    new_yuji->addSkill(new NewGuhuo);
    new_yuji->addSkill(new NewGuhuoClear);
    related_skills.insertMulti("new_guhuo", "#newguhuo-clear");
    addMetaObject<NewGuhuoCard>();
    skills << new Chanyuan;
*/
}

ADD_PACKAGE(NewStandard)
