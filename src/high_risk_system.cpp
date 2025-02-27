#include "Player.h"
#include "Creature.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "Map.h"
#include "Pet.h"
#include "Item.h"
#include "Chat.h"
#include "GameObject.h" // For GO_STATE_READY and GAMEOBJECT_FLAGS

#define SPELL_SICKNESS 15007
#define GOB_CHEST 179697 // Heavy Junkbox, cleared loot table

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

    void OnPVPKill(Player* killer, Player* killed)
    {
        printf("OnPVPKill triggered for killer GUID: %u, killed GUID: %u\n", killer->GetGUID().GetCounter(), killed->GetGUID().GetCounter());
        ChatHandler(killer->GetSession()).PSendSysMessage("PVP Kill triggered!");
        if (!roll_chance_i(70))
        {
            printf("Failed 70%% roll chance\n");
            return;
        }

        ReskillCheck(killer, killed);
        printf("Passed ReskillCheck\n");

        if (!killed->IsAlive())
        {
            printf("Killed player is dead, proceeding\n");
            uint32 count = 0;

            GameObject* go = killer->SummonGameObject(GOB_CHEST, killed->GetPositionX(), killed->GetPositionY(), killed->GetPositionZ(), killed->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 300);
            if (go)
            {
                printf("Chest spawned successfully: Entry %u at X: %.2f, Y: %.2f, Z: %.2f\n", GOB_CHEST, killed->GetPositionX(), killed->GetPositionY(), killed->GetPositionZ());
                killer->AddGameObject(go);
                go->SetOwnerGUID(ObjectGuid::Empty);
                go->loot.clear(); // Ensure no default loot
                go->SetGoState(GO_STATE_READY); // Ensure it’s lootable
                go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE | GO_FLAG_DESTROYED | GO_FLAG_INTERACT_COND); // Ensure it’s interactive
                go->SetRespawnTime(300); // Set a 24-hour respawn time to prevent despawning
                printf("Chest state set to %u, flags: %u\n", go->GetGoState(), go->GetUInt32Value(GAMEOBJECT_FLAGS)); // Debug state and flags

                // Equipment slots
                for (uint8 i = 0; i < EQUIPMENT_SLOT_END && count < 2; ++i)
                {
                    if (Item* pItem = killed->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    {
                        if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON) // Keeps loot consistency for uncommon (2) and rare (3) items
                        {
                            std::string itemName = pItem->GetTemplate()->Name1;
                            printf("Removing equipped item: %s (Entry: %u, Slot: %u, Name for message: '%s')\n", 
                                   itemName.c_str(), pItem->GetEntry(), pItem->GetSlot(), itemName.c_str());
                            if (!itemName.empty())
                            {
                                // Use only SendSysMessage with string concatenation (no %s or hyperlinks)
                                std::string message = "|cffDA70D6You have lost your " + itemName;
                                ChatHandler(killed->GetSession()).SendSysMessage(message.c_str());
                            }
                            else
                            {
                                ChatHandler(killed->GetSession()).SendSysMessage("|cffDA70D6You have lost an item (Entry: %u, No Name)", pItem->GetEntry());
                                printf("Warning: Item name is empty for entry %u\n", pItem->GetEntry());
                            }
                            go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                            printf("Added item to chest loot: Entry %u, Name: %s\n", pItem->GetEntry(), itemName.c_str());
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
                        if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON) // Keeps loot consistency
                        {
                            std::string itemName = pItem->GetTemplate()->Name1;
                            printf("Removing inventory item: %s (Entry: %u, Slot: %u, Name for message: '%s')\n", 
                                   itemName.c_str(), pItem->GetEntry(), i, itemName.c_str());
                            if (!itemName.empty())
                            {
                                // Use only SendSysMessage with string concatenation
                                std::string message = "|cffDA70D6You have lost your " + itemName;
                                ChatHandler(killed->GetSession()).SendSysMessage(message.c_str());
                            }
                            else
                            {
                                ChatHandler(killed->GetSession()).SendSysMessage("|cffDA70D6You have lost an item (Entry: %u, No Name)", pItem->GetEntry());
                                printf("Warning: Item name is empty for entry %u\n", pItem->GetEntry());
                            }
                            go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                            printf("Added item to chest loot: Entry %u, Name: %s\n", pItem->GetEntry(), itemName.c_str());
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
                                if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON) // Keeps loot consistency
                                {
                                    std::string itemName = pItem->GetTemplate()->Name1;
                                    printf("Removing bag item: %s (Entry: %u, Bag: %u, Slot: %u, Name for message: '%s')\n", 
                                           itemName.c_str(), pItem->GetEntry(), i, j, itemName.c_str());
                                    if (!itemName.empty())
                                    {
                                        // Use only SendSysMessage with string concatenation
                                        std::string message = "|cffDA70D6You have lost your " + itemName;
                                        ChatHandler(killed->GetSession()).SendSysMessage(message.c_str());
                                    }
                                    else
                                    {
                                        ChatHandler(killed->GetSession()).SendSysMessage("|cffDA70D6You have lost an item (Entry: %u, No Name)", pItem->GetEntry());
                                        printf("Warning: Item name is empty for entry %u\n", pItem->GetEntry());
                                    }
                                    go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                                    printf("Added item to chest loot: Entry %u, Name: %s\n", pItem->GetEntry(), itemName.c_str());
                                    killed->DestroyItemCount(pItem->GetEntry(), pItem->GetCount(), true, false);
                                    count++;
                                }
                            }
                        }
                    }
                }

                printf("Chest loot count: %zu\n", go->loot.items.size());
            }
            else
            {
                printf("Failed to spawn chest with entry %u\n", GOB_CHEST);
            }
        }
        else
        {
            printf("Killed player is alive, skipping chest spawn\n");
        }
    }
};

void AddSC_HighRiskSystems()
{
    new HighRiskSystem();
}