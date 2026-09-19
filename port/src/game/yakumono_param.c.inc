
static void* stage_yakumono_param(MeleeHostHsdReader* reader, mh_u32 root)
{
    const mh_u32 size = melee_host_hsd_reader_extent(reader, root);
    mh_u32 at;
    mh_u32 target;

    for (at = 0; at < size; at += 4) {
        if (melee_host_hsd_reader_u32(reader, root + at) != 0) break;
    }
    if (at == size) {
        if (size == 0) {
            melee_host_hsd_reader_fail(reader, "yakumono_param has no room");
            return NULL;
        }
        void* block = melee_host_hsd_reader_allocate(reader, size, alignof(mh_u32));
        if (block) memset(block, 0, size);
        return block;
    }

    if (size == 0) {
        return NULL;
    }

    switch (size) {
        case 8:
        if (melee_host_hsd_reader_has_pointer(reader, root + 0x0)) {
            struct grBattle_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grBattle_YakumonoParam));
            if (melee_host_hsd_reader_pointer(reader, root + 0x0, &target)) {
                out->bg_curr_color_overlay = (void*)(uintptr_t)target;
            } else {
                out->bg_curr_color_overlay = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x4, &target)) {
                out->bg_prev_color_overlay = (void*)(uintptr_t)target;
            } else {
                out->bg_prev_color_overlay = NULL;
            }
            return out;
        }
        else {
            struct grZebesRoute_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grZebesRoute_YakumonoParam));
            out->camera_timer = (s32) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->zako_spawn_chance = (s32) melee_host_hsd_reader_u32(reader, root + 0x4);
            return out;
        }
        case 12:
        {
            struct grTGn_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grTGn_YakumonoParam));
            if (melee_host_hsd_reader_pointer(reader, root + 0x0, &target)) {
                out->x0 = (void*)(uintptr_t)target;
            } else {
                out->x0 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x4, &target)) {
                out->x4 = (void*)(uintptr_t)target;
            } else {
                out->x4 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x8, &target)) {
                out->x8 = (void*)(uintptr_t)target;
            } else {
                out->x8 = NULL;
            }
            return out;
        }
        case 16:
        if (!melee_host_hsd_reader_has_pointer(reader, root + 0x0)) {
            struct grTFalco_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grTFalco_YakumonoParam));
            out->unk_0 = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->unk_4 = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->unk_8 = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->unk_C = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0xC);
            return out;
        }
        if (!melee_host_hsd_reader_has_pointer(reader, root + 0x0)) {
            struct grTFox_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grTFox_YakumonoParam));
            out->unk0 = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->unk4 = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->unk8 = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->unkC = (void*)(uintptr_t) melee_host_hsd_reader_u32(reader, root + 0xC);
            return out;
        }
        else {
            struct grLast_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grLast_YakumonoParam));
            if (melee_host_hsd_reader_pointer(reader, root + 0x0, &target)) {
                out->x0 = (void*)(uintptr_t)target;
            } else {
                out->x0 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x4, &target)) {
                out->x4 = (void*)(uintptr_t)target;
            } else {
                out->x4 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x8, &target)) {
                out->x8 = (void*)(uintptr_t)target;
            } else {
                out->x8 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0xC, &target)) {
                out->xC = (void*)(uintptr_t)target;
            } else {
                out->xC = NULL;
            }
            return out;
        }
        case 32:
        {
            struct grGarden_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grGarden_YakumonoParam));
            out->x0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->x4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->x8 = (s32) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->xC = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
            out->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->x14 = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->x18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->x1C = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            return out;
        }
        case 36:
        if (melee_host_hsd_reader_has_pointer(reader, root + 0x4)) {
            struct grPushon_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grPushon_YakumonoParam));
            out->x0 = (s32) melee_host_hsd_reader_u32(reader, root + 0x0);
            if (melee_host_hsd_reader_pointer(reader, root + 0x4, &target)) {
                out->x4 = (void*)(uintptr_t)target;
            } else {
                out->x4 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x8, &target)) {
                out->x8 = (void*)(uintptr_t)target;
            } else {
                out->x8 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0xC, &target)) {
                out->xC = (void*)(uintptr_t)target;
            } else {
                out->xC = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x10, &target)) {
                out->x10 = (void*)(uintptr_t)target;
            } else {
                out->x10 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x14, &target)) {
                out->x14 = (void*)(uintptr_t)target;
            } else {
                out->x14 = NULL;
            }
            out->x18 = (u8) melee_host_hsd_reader_u8(reader, root + 0x18);
            return out;
        }
        else {
            struct grStory_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grStory_YakumonoParam));
            out->timer_min = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->timer_rand = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->spawnmany_rarity = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            for (int i = 0; i < 6; i++) out->vpos[i] = (f32) melee_host_hsd_reader_f32(reader, root + 0xC + i * 4);
            return out;
        }
        case 44:
        {
            struct grShrineRoute_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grShrineRoute_YakumonoParam));
            if (melee_host_hsd_reader_pointer(reader, root + 0x0, &target)) {
                out->x0 = (void*)(uintptr_t)target;
            } else {
                out->x0 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x4, &target)) {
                out->x4 = (void*)(uintptr_t)target;
            } else {
                out->x4 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x8, &target)) {
                out->x8 = (void*)(uintptr_t)target;
            } else {
                out->x8 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0xC, &target)) {
                out->xC = (void*)(uintptr_t)target;
            } else {
                out->xC = NULL;
            }
            out->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->x14 = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->x18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->x1C = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->x20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->x24 = (s32) melee_host_hsd_reader_u32(reader, root + 0x24);
            return out;
        }
        case 52:
        if (1) {
            struct grOldpupupu_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grOldpupupu_YakumonoParam));
            out->x0 = (u16) melee_host_hsd_reader_u16(reader, root + 0x0);
            out->x2 = (u16) melee_host_hsd_reader_u16(reader, root + 0x2);
            out->x4 = (u16) melee_host_hsd_reader_u16(reader, root + 0x4);
            out->x6 = (u16) melee_host_hsd_reader_u16(reader, root + 0x6);
            out->x8 = (s32) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->xC = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
            out->x10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->x14 = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->x18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->x1C = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->x20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->x24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->x28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            return out;
        }
        else {
            struct grKraid_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grKraid_YakumonoParam));
            out->map_time_min = (u32) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->map_time_max = (u32) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->map_time_acl = (s32) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->map_rot_spd_min = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->map_rot_spd_max = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->kraid_wait_time = (u32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->kraid_wait_time_add = (u32) melee_host_hsd_reader_u32(reader, root + 0x18);
            for (int i = 0; i < 6; i++) out->kraid_pos_x[i] = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C + i * 4);
            return out;
        }
        case 60:
        {
            struct grVenom_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grVenom_YakumonoParam));
            out->x0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->x4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->x8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->xC = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->x10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            for (int i = 0; i < 24; i++) out->x14[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x14 + i * 1);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            for (int i = 0; i < 4; i++) out->x30[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x30 + i * 1);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            if (melee_host_hsd_reader_pointer(reader, root + 0x38, &target)) {
                out->x38 = (void*)(uintptr_t)target;
            } else {
                out->x38 = NULL;
            }
            return out;
        }
        case 64:
        {
            struct grFlatzone_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grFlatzone_YakumonoParam));
            out->unk0 = (s32) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->unk4 = (s32) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->unk8 = (s32) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->unkC = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
            out->unk10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->unk14 = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->unk18 = (s32) melee_host_hsd_reader_u32(reader, root + 0x18);
            out->unk1C = (s32) melee_host_hsd_reader_u32(reader, root + 0x1C);
            out->unk20 = (s32) melee_host_hsd_reader_u32(reader, root + 0x20);
            out->unk24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->unk28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->unk2C = (s32) melee_host_hsd_reader_u32(reader, root + 0x2C);
            out->unk30 = (s32) melee_host_hsd_reader_u32(reader, root + 0x30);
            out->unk34 = (s32) melee_host_hsd_reader_u32(reader, root + 0x34);
            out->unk38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->unk3C = (s32) melee_host_hsd_reader_u32(reader, root + 0x3C);
            return out;
        }
        case 72:
        {
            struct grRCruise_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grRCruise_YakumonoParam));
            out->x0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->x4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->x8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->xC = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
            out->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->x14 = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->x18 = (s32) melee_host_hsd_reader_u32(reader, root + 0x18);
            out->x1C = (s32) melee_host_hsd_reader_u32(reader, root + 0x1C);
            out->x20 = (s32) melee_host_hsd_reader_u32(reader, root + 0x20);
            out->x24 = (s32) melee_host_hsd_reader_u32(reader, root + 0x24);
            out->x28 = (s32) melee_host_hsd_reader_u32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->x38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->x3C = (s32) melee_host_hsd_reader_u32(reader, root + 0x3C);
            out->x40 = (s32) melee_host_hsd_reader_u32(reader, root + 0x40);
            out->x44 = (s32) melee_host_hsd_reader_u32(reader, root + 0x44);
            return out;
        }
        case 76:
        if (1) {
            struct grFourside_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grFourside_YakumonoParam));
            out->heli_wait = (s32) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->heli_wait_add = (s32) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->heli_stay_time = (s32) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->crane_wait = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
            out->crane_wait_add = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->crane_iron_wait = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->crane_iron_wait_add = (s32) melee_host_hsd_reader_u32(reader, root + 0x18);
            out->crane_iron_up_min = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->crane_iron_up_max = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->crane_iron_down_min = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->crane_iron_down_max = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->crane_iron_spd = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->crane_iron_stop_acl = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->ufo_wait = (s32) melee_host_hsd_reader_u32(reader, root + 0x34);
            out->ufo_cs_offs = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->ufo_stay_time = (s32) melee_host_hsd_reader_u32(reader, root + 0x3C);
            out->ufo_stay_time_add = (s32) melee_host_hsd_reader_u32(reader, root + 0x40);
            out->ufo_challenge = (u16) melee_host_hsd_reader_u16(reader, root + 0x44);
            out->x46 = (u16) melee_host_hsd_reader_u16(reader, root + 0x46);
            out->x48 = (u16) melee_host_hsd_reader_u16(reader, root + 0x48);
            return out;
        }
        else {
            struct grInishie2_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grInishie2_YakumonoParam));
            out->unk0 = (u16) melee_host_hsd_reader_u16(reader, root + 0x0);
            out->unk2 = (u16) melee_host_hsd_reader_u16(reader, root + 0x2);
            out->unk4 = (u16) melee_host_hsd_reader_u16(reader, root + 0x4);
            out->unk6 = (u16) melee_host_hsd_reader_u16(reader, root + 0x6);
            out->unk8 = (u16) melee_host_hsd_reader_u16(reader, root + 0x8);
            out->unkA = (u16) melee_host_hsd_reader_u16(reader, root + 0xA);
            out->unkC = (u16) melee_host_hsd_reader_u16(reader, root + 0xC);
            out->unkE = (u16) melee_host_hsd_reader_u16(reader, root + 0xE);
            for (int i = 0; i < 2; i++) out->unk10[i] = (u16) melee_host_hsd_reader_u16(reader, root + 0x10 + i * 2);
            out->unk2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->unk48 = (u16) melee_host_hsd_reader_u16(reader, root + 0x48);
            out->unk4A = (u16) melee_host_hsd_reader_u16(reader, root + 0x4A);
            return out;
        }
        case 80:
        if (melee_host_hsd_reader_has_pointer(reader, root + 0x0)) {
            struct grMc_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grMc_YakumonoParam));
            if (melee_host_hsd_reader_pointer(reader, root + 0x0, &target)) {
                out->x0 = (void*)(uintptr_t)target;
            } else {
                out->x0 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x4, &target)) {
                out->x4 = (void*)(uintptr_t)target;
            } else {
                out->x4 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0x8, &target)) {
                out->x8 = (void*)(uintptr_t)target;
            } else {
                out->x8 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0xC, &target)) {
                out->xC = (void*)(uintptr_t)target;
            } else {
                out->xC = NULL;
            }
            for (int i = 0; i < 28; i++) out->pad10[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x10 + i * 1);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->x38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            return out;
        }
        else {
            struct grBigBlueRoute_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grBigBlueRoute_YakumonoParam));
            out->x0 = (s32) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->x4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            for (int i = 0; i < 24; i++) out->pad_8[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x8 + i * 1);
            out->x20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            for (int i = 0; i < 24; i++) out->pad_24[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x24 + i * 1);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            return out;
        }
        case 84:
        if (1) {
            struct grIzumi_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grIzumi_YakumonoParam));
            out->x0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->x4 = (s32) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->x8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->xC = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->x10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->x14 = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->x18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->x1C = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->x20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->x24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->x28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->x38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            out->x50 = (f32) melee_host_hsd_reader_f32(reader, root + 0x50);
            return out;
        }
        if (1) {
            struct grInishie1_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grInishie1_YakumonoParam));
            out->unk0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->unk4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->unk8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->unkC = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->unk10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->unk14 = (u16) melee_host_hsd_reader_u16(reader, root + 0x14);
            out->unk16 = (u16) melee_host_hsd_reader_u16(reader, root + 0x16);
            out->unk18 = (u16) melee_host_hsd_reader_u16(reader, root + 0x18);
            out->unk1A = (u16) melee_host_hsd_reader_u16(reader, root + 0x1A);
            out->unk1C = (u16) melee_host_hsd_reader_u16(reader, root + 0x1C);
            out->unk1E = (u16) melee_host_hsd_reader_u16(reader, root + 0x1E);
            out->unk20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->unk24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->unk28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->unk44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->unk48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->unk4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            out->unk50 = (f32) melee_host_hsd_reader_f32(reader, root + 0x50);
            return out;
        }
        else {
            struct grPStadium_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grPStadium_YakumonoParam));
            out->x0 = (s32) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->x4 = (s32) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->x8 = (s32) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->xC = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
            out->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->x14 = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->x18 = (s32) melee_host_hsd_reader_u32(reader, root + 0x18);
            out->r = (u8) melee_host_hsd_reader_u8(reader, root + 0x1C);
            out->g = (u8) melee_host_hsd_reader_u8(reader, root + 0x1D);
            out->b = (u8) melee_host_hsd_reader_u8(reader, root + 0x1E);
            out->x20 = (u32) melee_host_hsd_reader_u32(reader, root + 0x20);
            out->x24 = (u32) melee_host_hsd_reader_u32(reader, root + 0x24);
            out->x28 = (u32) melee_host_hsd_reader_u32(reader, root + 0x28);
            out->x2C = (u32) melee_host_hsd_reader_u32(reader, root + 0x2C);
            out->x30 = (u32) melee_host_hsd_reader_u32(reader, root + 0x30);
            out->x34 = (u32) melee_host_hsd_reader_u32(reader, root + 0x34);
            out->x38 = (u32) melee_host_hsd_reader_u32(reader, root + 0x38);
            out->x3C = (u32) melee_host_hsd_reader_u32(reader, root + 0x3C);
            out->x40 = (u32) melee_host_hsd_reader_u32(reader, root + 0x40);
            out->x44 = (u32) melee_host_hsd_reader_u32(reader, root + 0x44);
            out->x48 = (u16) melee_host_hsd_reader_u16(reader, root + 0x48);
            out->x4A = (u16) melee_host_hsd_reader_u16(reader, root + 0x4A);
            out->x4C = (u16) melee_host_hsd_reader_u16(reader, root + 0x4C);
            out->x4E = (u16) melee_host_hsd_reader_u16(reader, root + 0x4E);
            out->x50 = (u16) melee_host_hsd_reader_u16(reader, root + 0x50);
            return out;
        }
        case 112:
        {
            struct grOldKongo_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grOldKongo_YakumonoParam));
            out->rframe_bird_wait_a = (u16) melee_host_hsd_reader_u16(reader, root + 0x0);
            out->rframe_bird_wait_b = (u16) melee_host_hsd_reader_u16(reader, root + 0x2);
            out->rrange_bird_random_offset_y = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->rframe_barrel_shoot_a = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->rframe_barrel_shoot_b = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->rframe_barrel_in = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->rframe_barrel_wait_a = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->rframe_barrel_wait_b = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->rspeed_barrel_rot_accel = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->rspeed_barrel_rot_max = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->rframe_barrel_roll_a = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->rframe_barrel_roll_b = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->rrate_barrel_ld = (u16) melee_host_hsd_reader_u16(reader, root + 0x2C);
            out->rrate_barrel_l = (u16) melee_host_hsd_reader_u16(reader, root + 0x2E);
            out->rrate_barrel_lu = (u16) melee_host_hsd_reader_u16(reader, root + 0x30);
            out->rrate_barrel_u = (u16) melee_host_hsd_reader_u16(reader, root + 0x32);
            out->rrate_barrel_ru = (u16) melee_host_hsd_reader_u16(reader, root + 0x34);
            out->rrate_barrel_r = (u16) melee_host_hsd_reader_u16(reader, root + 0x36);
            out->rrate_barrel_rd = (u16) melee_host_hsd_reader_u16(reader, root + 0x38);
            out->rrate_barrel_d = (u16) melee_host_hsd_reader_u16(reader, root + 0x3A);
            out->rframe_barrel_interval_a = (s32) melee_host_hsd_reader_u32(reader, root + 0x3C);
            out->rframe_barrel_interval_b = (s32) melee_host_hsd_reader_u32(reader, root + 0x40);
            out->rspeed_barrel_move_accel = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->rspeed_barrel_move_max = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->rframe_barrel_stop_a = (s32) melee_host_hsd_reader_u32(reader, root + 0x4C);
            out->rframe_barrel_stop_b = (s32) melee_host_hsd_reader_u32(reader, root + 0x50);
            out->rpower_barrel_attack = (s32) melee_host_hsd_reader_u32(reader, root + 0x54);
            out->rvector_barrel_attack = (s32) melee_host_hsd_reader_u32(reader, root + 0x58);
            out->rreff_barrel_attack = (s32) melee_host_hsd_reader_u32(reader, root + 0x5C);
            out->rrfix_barrel_attack = (s32) melee_host_hsd_reader_u32(reader, root + 0x60);
            out->rradd_barrel_attack = (s32) melee_host_hsd_reader_u32(reader, root + 0x64);
            out->x68 = (s32) melee_host_hsd_reader_u32(reader, root + 0x68);
            if (melee_host_hsd_reader_pointer(reader, root + 0x6C, &target)) {
                out->x6C = (void*)(uintptr_t)target;
            } else {
                out->x6C = NULL;
            }
            return out;
        }
        case 124:
        {
            struct grGreens_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grGreens_YakumonoParam));
            out->x0_blockTimerMin = (s32) melee_host_hsd_reader_u32(reader, root + 0x0);
            out->x4_blockTimerMax = (s32) melee_host_hsd_reader_u32(reader, root + 0x4);
            out->x8_blockBombChance = (s32) melee_host_hsd_reader_u32(reader, root + 0x8);
            out->xC = (s32) melee_host_hsd_reader_u32(reader, root + 0xC);
            out->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->x14 = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->x18 = (s32) melee_host_hsd_reader_u32(reader, root + 0x18);
            out->x1C = (s32) melee_host_hsd_reader_u32(reader, root + 0x1C);
            out->x20 = (s32) melee_host_hsd_reader_u32(reader, root + 0x20);
            out->x24 = (s32) melee_host_hsd_reader_u32(reader, root + 0x24);
            out->x28 = (s32) melee_host_hsd_reader_u32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34_windTimerMin = (s32) melee_host_hsd_reader_u32(reader, root + 0x34);
            out->x38_windTimerMax = (s32) melee_host_hsd_reader_u32(reader, root + 0x38);
            out->x3C_windSpeed = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40_left = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44_right = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48_top = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C_bottom = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            out->x50 = (f32) melee_host_hsd_reader_f32(reader, root + 0x50);
            out->x54 = (f32) melee_host_hsd_reader_f32(reader, root + 0x54);
            out->x58 = (f32) melee_host_hsd_reader_f32(reader, root + 0x58);
            out->x5C = (s32) melee_host_hsd_reader_u32(reader, root + 0x5C);
            out->x60 = (s32) melee_host_hsd_reader_u32(reader, root + 0x60);
            out->x64 = (s32) melee_host_hsd_reader_u32(reader, root + 0x64);
            out->x68 = (s32) melee_host_hsd_reader_u32(reader, root + 0x68);
            out->x6C = (f32) melee_host_hsd_reader_f32(reader, root + 0x6C);
            out->x70 = (f32) melee_host_hsd_reader_f32(reader, root + 0x70);
            out->x74 = (f32) melee_host_hsd_reader_f32(reader, root + 0x74);
            out->x78 = (f32) melee_host_hsd_reader_f32(reader, root + 0x78);
            return out;
        }
        case 140:
        {
            struct grCorneria_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grCorneria_YakumonoParam));
            out->x0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->x4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->x8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->xC = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->x10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->x14 = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->x18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->x1C = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->x20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->x24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->x28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->x38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            for (int i = 0; i < 24; i++) out->pad50[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x50 + i * 1);
            out->x68 = (f32) melee_host_hsd_reader_f32(reader, root + 0x68);
            for (int i = 0; i < 4; i++) out->pad6C[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x6C + i * 1);
            out->x70 = (f32) melee_host_hsd_reader_f32(reader, root + 0x70);
            out->x74 = (s32) melee_host_hsd_reader_u32(reader, root + 0x74);
            out->x78 = (s32) melee_host_hsd_reader_u32(reader, root + 0x78);
            out->x7C = (s32) melee_host_hsd_reader_u32(reader, root + 0x7C);
            out->x80 = (s32) melee_host_hsd_reader_u32(reader, root + 0x80);
            if (melee_host_hsd_reader_pointer(reader, root + 0x84, &target)) {
                out->x84 = (void*)(uintptr_t)target;
            } else {
                out->x84 = NULL;
            }
            out->x88 = (f32) melee_host_hsd_reader_f32(reader, root + 0x88);
            return out;
        }
        case 188:
        {
            struct grKongo_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grKongo_YakumonoParam));
            out->unk0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->unk4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->unk8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->unkC = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->unk10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->unk14 = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->unk18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->unk1C = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->unk20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->unk24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->unk28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->unk2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->unk30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->unk34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->unk38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->unk3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->unk40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->unk44 = (u16) melee_host_hsd_reader_u16(reader, root + 0x44);
            out->unk46 = (u16) melee_host_hsd_reader_u16(reader, root + 0x46);
            out->unk48 = (u16) melee_host_hsd_reader_u16(reader, root + 0x48);
            out->unk4A = (u16) melee_host_hsd_reader_u16(reader, root + 0x4A);
            out->unk4C = (u16) melee_host_hsd_reader_u16(reader, root + 0x4C);
            out->unk4E = (u16) melee_host_hsd_reader_u16(reader, root + 0x4E);
            out->unk50 = (u16) melee_host_hsd_reader_u16(reader, root + 0x50);
            out->unk52 = (u16) melee_host_hsd_reader_u16(reader, root + 0x52);
            out->unk54 = (f32) melee_host_hsd_reader_f32(reader, root + 0x54);
            out->unk58 = (f32) melee_host_hsd_reader_f32(reader, root + 0x58);
            out->unk5C = (f32) melee_host_hsd_reader_f32(reader, root + 0x5C);
            out->unk60 = (f32) melee_host_hsd_reader_f32(reader, root + 0x60);
            out->unk64 = (s32) melee_host_hsd_reader_u32(reader, root + 0x64);
            out->unk68 = (s32) melee_host_hsd_reader_u32(reader, root + 0x68);
            out->unk6C = (f32) melee_host_hsd_reader_f32(reader, root + 0x6C);
            out->unk70 = (f32) melee_host_hsd_reader_f32(reader, root + 0x70);
            out->unk74 = (f32) melee_host_hsd_reader_f32(reader, root + 0x74);
            out->unk78 = (f32) melee_host_hsd_reader_f32(reader, root + 0x78);
            out->unk7C = (f32) melee_host_hsd_reader_f32(reader, root + 0x7C);
            out->unk80 = (f32) melee_host_hsd_reader_f32(reader, root + 0x80);
            if (melee_host_hsd_reader_pointer(reader, root + 0x84, &target)) {
                out->unk84 = (void*)(uintptr_t)target;
            } else {
                out->unk84 = NULL;
            }
            out->unk88 = (f32) melee_host_hsd_reader_f32(reader, root + 0x88);
            out->unk8C = (f32) melee_host_hsd_reader_f32(reader, root + 0x8C);
            out->unk90 = (f32) melee_host_hsd_reader_f32(reader, root + 0x90);
            out->unk94 = (f32) melee_host_hsd_reader_f32(reader, root + 0x94);
            out->unk98 = (f32) melee_host_hsd_reader_f32(reader, root + 0x98);
            out->unk9C = (f32) melee_host_hsd_reader_f32(reader, root + 0x9C);
            out->unkA0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xA0);
            out->unkA4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xA4);
            out->unkA8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xA8);
            out->unkAC = (f32) melee_host_hsd_reader_f32(reader, root + 0xAC);
            out->unkB0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xB0);
            out->unkB4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xB4);
            out->unkB8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xB8);
            return out;
        }
        case 212:
        {
            struct grIceMt_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grIceMt_YakumonoParam));
            out->x0 = (u16) melee_host_hsd_reader_u16(reader, root + 0x0);
            out->x2 = (u16) melee_host_hsd_reader_u16(reader, root + 0x2);
            out->x4 = (u16) melee_host_hsd_reader_u16(reader, root + 0x4);
            out->x8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->xC = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->x10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->x14 = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->x18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            out->x1C = (f32) melee_host_hsd_reader_f32(reader, root + 0x1C);
            out->x20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->x24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->x28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (u16) melee_host_hsd_reader_u16(reader, root + 0x34);
            out->x36 = (u16) melee_host_hsd_reader_u16(reader, root + 0x36);
            out->x38 = (u16) melee_host_hsd_reader_u16(reader, root + 0x38);
            out->x3A = (u16) melee_host_hsd_reader_u16(reader, root + 0x3A);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            out->x50 = (f32) melee_host_hsd_reader_f32(reader, root + 0x50);
            out->x54 = (f32) melee_host_hsd_reader_f32(reader, root + 0x54);
            out->x58 = (f32) melee_host_hsd_reader_f32(reader, root + 0x58);
            out->x5C = (f32) melee_host_hsd_reader_f32(reader, root + 0x5C);
            out->x60 = (f32) melee_host_hsd_reader_f32(reader, root + 0x60);
            out->x64 = (f32) melee_host_hsd_reader_f32(reader, root + 0x64);
            out->x68 = (f32) melee_host_hsd_reader_f32(reader, root + 0x68);
            out->x6C = (f32) melee_host_hsd_reader_f32(reader, root + 0x6C);
            out->x70 = (f32) melee_host_hsd_reader_f32(reader, root + 0x70);
            out->x74 = (f32) melee_host_hsd_reader_f32(reader, root + 0x74);
            out->x78 = (f32) melee_host_hsd_reader_f32(reader, root + 0x78);
            out->x7C = (f32) melee_host_hsd_reader_f32(reader, root + 0x7C);
            out->x80 = (f32) melee_host_hsd_reader_f32(reader, root + 0x80);
            out->x84 = (f32) melee_host_hsd_reader_f32(reader, root + 0x84);
            out->x88 = (f32) melee_host_hsd_reader_f32(reader, root + 0x88);
            out->x8C = (f32) melee_host_hsd_reader_f32(reader, root + 0x8C);
            out->x90 = (f32) melee_host_hsd_reader_f32(reader, root + 0x90);
            out->x94 = (f32) melee_host_hsd_reader_f32(reader, root + 0x94);
            out->ft_max_y = (u16) melee_host_hsd_reader_u16(reader, root + 0x98);
            out->x9E = (u16) melee_host_hsd_reader_u16(reader, root + 0x9E);
            out->x9C = (f32) melee_host_hsd_reader_f32(reader, root + 0xA0);
            out->xA0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xA4);
            out->xA4 = (u16) melee_host_hsd_reader_u16(reader, root + 0xA8);
            out->xA6 = (u16) melee_host_hsd_reader_u16(reader, root + 0xAA);
            out->xA8 = (u16) melee_host_hsd_reader_u16(reader, root + 0xAC);
            if (melee_host_hsd_reader_pointer(reader, root + 0xAE, &target)) {
                out->field_ixs = (void*)(uintptr_t)target;
            } else {
                out->field_ixs = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0xB2, &target)) {
                out->xB0 = (void*)(uintptr_t)target;
            } else {
                out->xB0 = NULL;
            }
            if (melee_host_hsd_reader_pointer(reader, root + 0xB6, &target)) {
                out->xB4 = (void*)(uintptr_t)target;
            } else {
                out->xB4 = NULL;
            }
            out->xB8 = (u16) melee_host_hsd_reader_u16(reader, root + 0xBA);
            out->pad = (u16) melee_host_hsd_reader_u16(reader, root + 0xBC);
            out->xC0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xC2);
            out->xC4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xC6);
            out->xC8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xCA);
            out->xCC = (f32) melee_host_hsd_reader_f32(reader, root + 0xCE);
            return out;
        }
        case 280:
        {
            struct grZe_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grZe_YakumonoParam));
            out->x00 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->x04 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->x08 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->x0C = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            for (int i = 0; i < 28; i++) out->pad_14[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x14 + i * 1);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->x38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            out->x50 = (f32) melee_host_hsd_reader_f32(reader, root + 0x50);
            out->x54 = (f32) melee_host_hsd_reader_f32(reader, root + 0x54);
            out->x58 = (f32) melee_host_hsd_reader_f32(reader, root + 0x58);
            out->x5C = (f32) melee_host_hsd_reader_f32(reader, root + 0x5C);
            out->x60 = (f32) melee_host_hsd_reader_f32(reader, root + 0x60);
            out->x64 = (f32) melee_host_hsd_reader_f32(reader, root + 0x64);
            out->x68 = (f32) melee_host_hsd_reader_f32(reader, root + 0x68);
            out->x6C = (f32) melee_host_hsd_reader_f32(reader, root + 0x6C);
            out->x70 = (f32) melee_host_hsd_reader_f32(reader, root + 0x70);
            out->x74 = (f32) melee_host_hsd_reader_f32(reader, root + 0x74);
            out->x78 = (f32) melee_host_hsd_reader_f32(reader, root + 0x78);
            out->x7C = (f32) melee_host_hsd_reader_f32(reader, root + 0x7C);
            out->x80 = (f32) melee_host_hsd_reader_f32(reader, root + 0x80);
            out->x84 = (f32) melee_host_hsd_reader_f32(reader, root + 0x84);
            out->x88 = (f32) melee_host_hsd_reader_f32(reader, root + 0x88);
            out->x8C = (f32) melee_host_hsd_reader_f32(reader, root + 0x8C);
            out->x90 = (f32) melee_host_hsd_reader_f32(reader, root + 0x90);
            out->x94 = (f32) melee_host_hsd_reader_f32(reader, root + 0x94);
            out->x98 = (f32) melee_host_hsd_reader_f32(reader, root + 0x98);
            out->x9C = (f32) melee_host_hsd_reader_f32(reader, root + 0x9C);
            return out;
        }
        case 324:
        if (melee_host_hsd_reader_has_pointer(reader, root + 0x114)) {
            struct grCastle_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grCastle_YakumonoParam));
            out->x0 = (u16) melee_host_hsd_reader_u16(reader, root + 0x0);
            out->x2 = (u16) melee_host_hsd_reader_u16(reader, root + 0x2);
            out->x4 = (u16) melee_host_hsd_reader_u16(reader, root + 0x4);
            out->x6 = (u16) melee_host_hsd_reader_u16(reader, root + 0x6);
            out->x8 = (u16) melee_host_hsd_reader_u16(reader, root + 0x8);
            out->xA = (u16) melee_host_hsd_reader_u16(reader, root + 0xA);
            out->xC = (u16) melee_host_hsd_reader_u16(reader, root + 0xC);
            out->xE = (u16) melee_host_hsd_reader_u16(reader, root + 0xE);
            out->x10 = (f32) melee_host_hsd_reader_f32(reader, root + 0x10);
            out->x14 = (f32) melee_host_hsd_reader_f32(reader, root + 0x14);
            out->x18 = (f32) melee_host_hsd_reader_f32(reader, root + 0x18);
            for (int i = 0; i < 4; i++) out->pad_x1C[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x1C + i * 1);
            out->x20 = (f32) melee_host_hsd_reader_f32(reader, root + 0x20);
            out->x24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->x28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->x38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (u16) melee_host_hsd_reader_u16(reader, root + 0x40);
            out->x42 = (u16) melee_host_hsd_reader_u16(reader, root + 0x42);
            out->x44 = (u16) melee_host_hsd_reader_u16(reader, root + 0x44);
            for (int i = 0; i < 2; i++) out->pad_x46[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x46 + i * 1);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            out->x50 = (f32) melee_host_hsd_reader_f32(reader, root + 0x50);
            out->x54 = (u16) melee_host_hsd_reader_u16(reader, root + 0x54);
            for (int i = 0; i < 2; i++) out->pad_x56[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x56 + i * 1);
            out->x58 = (u16) melee_host_hsd_reader_u16(reader, root + 0x58);
            for (int i = 0; i < 2; i++) out->pad_x5A[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x5A + i * 1);
            out->x110 = (f32) melee_host_hsd_reader_f32(reader, root + 0x110);
            if (melee_host_hsd_reader_pointer(reader, root + 0x114, &target)) {
                out->x114 = (void*)(uintptr_t)target;
            } else {
                out->x114 = NULL;
            }
            out->x118 = (f32) melee_host_hsd_reader_f32(reader, root + 0x118);
            out->x11C = (f32) melee_host_hsd_reader_f32(reader, root + 0x11C);
            out->x120 = (f32) melee_host_hsd_reader_f32(reader, root + 0x120);
            out->x124 = (f32) melee_host_hsd_reader_f32(reader, root + 0x124);
            for (int i = 0; i < 4; i++) out->pad_x128[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x128 + i * 1);
            for (int i = 0; i < 4; i++) out->x12C[i] = (u16) melee_host_hsd_reader_u16(reader, root + 0x12C + i * 2);
            out->x134 = (f32) melee_host_hsd_reader_f32(reader, root + 0x134);
            out->x138 = (f32) melee_host_hsd_reader_f32(reader, root + 0x138);
            out->x13C = (f32) melee_host_hsd_reader_f32(reader, root + 0x13C);
            out->x140 = (f32) melee_host_hsd_reader_f32(reader, root + 0x140);
            return out;
        }
        else {
            struct grBb_YakumonoParam* out = melee_host_hsd_reader_allocate(reader, sizeof(*out), alignof(struct grBb_YakumonoParam));
            out->x0 = (f32) melee_host_hsd_reader_f32(reader, root + 0x0);
            out->x4 = (f32) melee_host_hsd_reader_f32(reader, root + 0x4);
            out->x8 = (f32) melee_host_hsd_reader_f32(reader, root + 0x8);
            out->xC = (f32) melee_host_hsd_reader_f32(reader, root + 0xC);
            out->x10 = (s32) melee_host_hsd_reader_u32(reader, root + 0x10);
            out->x14 = (s32) melee_host_hsd_reader_u32(reader, root + 0x14);
            out->x18 = (s32) melee_host_hsd_reader_u32(reader, root + 0x18);
            out->x1C = (s32) melee_host_hsd_reader_u32(reader, root + 0x1C);
            out->x20 = (s32) melee_host_hsd_reader_u32(reader, root + 0x20);
            out->x24 = (f32) melee_host_hsd_reader_f32(reader, root + 0x24);
            out->x28 = (f32) melee_host_hsd_reader_f32(reader, root + 0x28);
            out->x2C = (f32) melee_host_hsd_reader_f32(reader, root + 0x2C);
            out->x30 = (f32) melee_host_hsd_reader_f32(reader, root + 0x30);
            out->x34 = (f32) melee_host_hsd_reader_f32(reader, root + 0x34);
            out->x38 = (f32) melee_host_hsd_reader_f32(reader, root + 0x38);
            out->x3C = (f32) melee_host_hsd_reader_f32(reader, root + 0x3C);
            out->x40 = (f32) melee_host_hsd_reader_f32(reader, root + 0x40);
            out->x44 = (f32) melee_host_hsd_reader_f32(reader, root + 0x44);
            out->x48 = (f32) melee_host_hsd_reader_f32(reader, root + 0x48);
            out->x4C = (f32) melee_host_hsd_reader_f32(reader, root + 0x4C);
            out->x50 = (f32) melee_host_hsd_reader_f32(reader, root + 0x50);
            out->x54 = (f32) melee_host_hsd_reader_f32(reader, root + 0x54);
            out->x58 = (f32) melee_host_hsd_reader_f32(reader, root + 0x58);
            out->x5C = (f32) melee_host_hsd_reader_f32(reader, root + 0x5C);
            out->x60 = (f32) melee_host_hsd_reader_f32(reader, root + 0x60);
            for (int i = 0; i < 4; i++) out->pad64[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x64 + i * 1);
            out->x68 = (f32) melee_host_hsd_reader_f32(reader, root + 0x68);
            out->x6C = (f32) melee_host_hsd_reader_f32(reader, root + 0x6C);
            out->x70 = (f32) melee_host_hsd_reader_f32(reader, root + 0x70);
            out->x74 = (f32) melee_host_hsd_reader_f32(reader, root + 0x74);
            out->x78 = (f32) melee_host_hsd_reader_f32(reader, root + 0x78);
            out->x7C = (f32) melee_host_hsd_reader_f32(reader, root + 0x7C);
            out->x80 = (f32) melee_host_hsd_reader_f32(reader, root + 0x80);
            out->x84 = (f32) melee_host_hsd_reader_f32(reader, root + 0x84);
            out->x88 = (s32) melee_host_hsd_reader_u32(reader, root + 0x88);
            out->x8C = (s32) melee_host_hsd_reader_u32(reader, root + 0x8C);
            out->x90 = (f32) melee_host_hsd_reader_f32(reader, root + 0x90);
            out->x94 = (f32) melee_host_hsd_reader_f32(reader, root + 0x94);
            out->x98 = (f32) melee_host_hsd_reader_f32(reader, root + 0x98);
            out->x9C = (f32) melee_host_hsd_reader_f32(reader, root + 0x9C);
            out->xA0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xA0);
            out->xA4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xA4);
            out->xA8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xA8);
            out->xAC = (f32) melee_host_hsd_reader_f32(reader, root + 0xAC);
            out->xB0 = (s32) melee_host_hsd_reader_u32(reader, root + 0xB0);
            out->xB4 = (s32) melee_host_hsd_reader_u32(reader, root + 0xB4);
            out->xB8 = (s32) melee_host_hsd_reader_u32(reader, root + 0xB8);
            out->xBC = (f32) melee_host_hsd_reader_f32(reader, root + 0xBC);
            out->xC0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xC0);
            out->xC4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xC4);
            out->xC8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xC8);
            out->xCC = (f32) melee_host_hsd_reader_f32(reader, root + 0xCC);
            out->xD0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xD0);
            out->xD4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xD4);
            out->xD8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xD8);
            out->xDC = (s32) melee_host_hsd_reader_u32(reader, root + 0xDC);
            out->xE0 = (s32) melee_host_hsd_reader_u32(reader, root + 0xE0);
            out->xE4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xE4);
            out->xE8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xE8);
            out->xEC = (f32) melee_host_hsd_reader_f32(reader, root + 0xEC);
            out->xF0 = (f32) melee_host_hsd_reader_f32(reader, root + 0xF0);
            out->xF4 = (f32) melee_host_hsd_reader_f32(reader, root + 0xF4);
            out->xF8 = (f32) melee_host_hsd_reader_f32(reader, root + 0xF8);
            out->xFC = (f32) melee_host_hsd_reader_f32(reader, root + 0xFC);
            out->x100 = (f32) melee_host_hsd_reader_f32(reader, root + 0x100);
            out->x104 = (f32) melee_host_hsd_reader_f32(reader, root + 0x104);
            out->x108 = (f32) melee_host_hsd_reader_f32(reader, root + 0x108);
            out->x10C = (s32) melee_host_hsd_reader_u32(reader, root + 0x10C);
            out->x110 = (s32) melee_host_hsd_reader_u32(reader, root + 0x110);
            for (int i = 0; i < 8; i++) out->pad114[i] = (u8) melee_host_hsd_reader_u8(reader, root + 0x114 + i * 1);
            out->x11C = (s32) melee_host_hsd_reader_u32(reader, root + 0x11C);
            out->x120 = (s32) melee_host_hsd_reader_u32(reader, root + 0x120);
            out->x124 = (f32) melee_host_hsd_reader_f32(reader, root + 0x124);
            out->x128 = (f32) melee_host_hsd_reader_f32(reader, root + 0x128);
            out->x12C = (f32) melee_host_hsd_reader_f32(reader, root + 0x12C);
            out->x130 = (f32) melee_host_hsd_reader_f32(reader, root + 0x130);
            out->x134_translate.x = melee_host_hsd_reader_f32(reader, root + 0x134 + 0x0);
            out->x134_translate.y = melee_host_hsd_reader_f32(reader, root + 0x134 + 0x4);
            out->x134_translate.z = melee_host_hsd_reader_f32(reader, root + 0x134 + 0x8);
            out->x140_scale = (f32) melee_host_hsd_reader_f32(reader, root + 0x140);
            return out;
        }
        default: {
            void* block = melee_host_hsd_reader_allocate(reader, size, alignof(mh_u32));
            if (block) memset(block, 0, size);
            return block;
        }
    }
}
