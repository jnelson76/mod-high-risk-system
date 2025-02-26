#include "Player.h"
#include "Creature.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "Map.h"
#include "Pet.h"
#include "Item.h"
#include "Chat.h"

#define SPELL_SICKNESS 15007
#define GOB_CHEST 176304 // Unlocked chest, no default loot

void ReskillCheck(Player* killer, Player* killed)
{
    if (killer->GetSession()->GetRemoteAddress() == killed->GetSession()->GetRemoteAddress() || killer->GetGUID() == killed->GetGUID())
        return;
    if (!killer->GetGUID().IsPlayer())
        return;
    if (killed->HasAura(SPELL_SICKNESS))
        return;
    if (killer->GetLevel() - 5 >= killed->GetLevel())
        return;
    AreaTableEntry const* area = sAreaTableStore.LookupEntry(killed->GetAreaId());
    AreaTableEntry const* area2 = sAreaTableStore.LookupEntry(killer->GetAreaId());
    if (area->IsSanctuary() || area2->IsSanctuary())
        return;
}

class HighRiskSystem : public PlayerScript
{
public:
    HighRiskSystem() : PlayerScript("HighRiskSystem") {}

    void OnPVPKill(Player* killer, Player* killed) override
    {
        if (!roll_chance_i(70))
            return;

        ReskillCheck(killer, killed);

        if (!killed->IsAlive())
        {
            uint32 count = 0;

            if (GameObject* go = killer->SummonGameObject(GOB_CHEST, killed->GetPositionX(), killed->GetPositionY(), killed->GetPositionZ(), killed->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 300))
            {
                killer->AddGameObject(go);
                go->SetOwnerGUID(ObjectGuid::Empty);
                go->loot.clear(); // Clear any potential default loot as a precaution

                // Equipment slots
                for (uint8 i = 0; i < EQUIPMENT_SLOT_END && count < 2; ++i)
                {
                    if (Item* pItem = killed->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    {
                        if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON)
                        {
                            std::string itemName = pItem->GetTemplate()->Name1;
                            printf("Removing equipped item: %s (Entry: %u, Slot: %u)\n", itemName.c_str(), pItem->GetEntry(), pItem->GetSlot());
                            ChatHandler(killed->GetSession()).PSendSysMessage("|cffDA70D6You have lost your |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r",
                                pItem->GetEntry(), itemName.c_str());
                            go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                            killed->DestroyItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                            count++;
                        }
                    }
                }

                // Main inventory
                for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END && count < 2; ++i)
                {
                    if (Item* pItem = killed->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    {
                        if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON)
                        {
                            std::string itemName = pItem->GetTemplate()->Name1;
                            printf("Removing inventory item: %s (Entry: %u, Slot: %u)\n", itemName.c_str(), pItem->GetEntry(), i);
                            ChatHandler(killed->GetSession()).PSendSysMessage("|cffDA70D6You have lost your |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r",
                                pItem->GetEntry(), itemName.c_str());
                            go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                            killed->DestroyItemCount(pItem->GetEntry(), pItem->GetCount(), true, false);
                            count++;
                        }
                    }
                }

                // Bags
                for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END && count < 2; ++i)
                {
                    if (Bag* bag = killed->GetBagByPos(i))
                    {
                        for (uint32 j = 0; j < bag->GetBagSize() && count < 2; ++j)
                        {
                            if (Item* pItem = killed->GetItemByPos(i, j))
                            {
                                if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON)
                                {
                                    std::string itemName = pItem->GetTemplate()->Name1;
                                    printf("Removing bag item: %s (Entry: %u, Bag: %u, Slot: %u)\n", itemName.c_str(), pItem->GetEntry(), i, j);
                                    ChatHandler(killed->GetSession()).PSendSysMessage("|cffDA70D6You have lost your |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r",
                                        pItem->GetEntry(), itemName.c_str());
                                    go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                                    killed->DestroyItemCount(pItem->GetEntry(), pItem->GetCount(), true, false);
                                    count++;
                                }
                            }
                        }
                    }
                }

                printf("Chest loot count: %u\n", go->loot.items.size()); // Debug final loot count
            }
        }
    }
};

void AddSC_HighRiskSystems()
{
    new HighRiskSystem();
}