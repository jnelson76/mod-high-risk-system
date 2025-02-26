void OnPVPKill(Player* killer, Player* killed)
{
    if (!roll_chance_i(70)) // 70% chance to proceed
        return;

    ReskillCheck(killer, killed); // Apply your checks

    if (!killed->IsAlive()) // Ensure player is dead
    {
        uint32 count = 0;

        // Spawn chest and clear its default loot
        if (GameObject* go = killer->SummonGameObject(GOB_CHEST, killed->GetPositionX(), killed->GetPositionY(), killed->GetPositionZ(), killed->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 300))
        {
            killer->AddGameObject(go);
            go->SetOwnerGUID(ObjectGuid::Empty);
            go->loot.clear(); // Clear default loot table

            // Equipment slots
            for (uint8 i = 0; i < EQUIPMENT_SLOT_END && count < 2; ++i)
            {
                if (Item* pItem = killed->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                {
                    if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON) // Only uncommon or higher
                    {
                        ChatHandler(killed->GetSession()).PSendSysMessage("|cffDA70D6You have lost your |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r", 
                            pItem->GetEntry(), pItem->GetTemplate()->Name1.c_str());

                        go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                        killed->DestroyItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                        count++;
                    }
                }
            }

            // Inventory and bags (if less than 2 items added)
            for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END && count < 2; ++i)
            {
                if (Item* pItem = killed->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                {
                    if (pItem->GetTemplate()->Quality >= ITEM_QUALITY_UNCOMMON)
                    {
                        ChatHandler(killed->GetSession()).PSendSysMessage("|cffDA70D6You have lost your |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r", 
                            pItem->GetEntry(), pItem->GetTemplate()->Name1.c_str());

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
                                ChatHandler(killed->GetSession()).PSendSysMessage("|cffDA70D6You have lost your |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r", 
                                    pItem->GetEntry(), pItem->GetTemplate()->Name1.c_str());

                                go->loot.AddItem(LootStoreItem(pItem->GetEntry(), 0, 100, 0, LOOT_MODE_DEFAULT, 0, 1, 1));
                                killed->DestroyItemCount(pItem->GetEntry(), pItem->GetCount(), true, false);
                                count++;
                            }
                        }
                    }
                }
            }
        }
    }
}