#ifndef _SIWIFI_MU_GROUP_H_
#define _SIWIFI_MU_GROUP_H_ 
#include <linux/workqueue.h>
#include <linux/semaphore.h>
struct siwifi_hw;
struct siwifi_sta;
#ifdef CONFIG_SIWIFI_MUMIMO_TX
struct siwifi_sta_group_info {
    struct list_head active;
    struct list_head update;
    u16 last_update;
    int cnt;
    u64 map;
    int traffic;
    u8 group;
};
struct siwifi_mu_group {
    struct list_head list;
    int group_id;
    int user_cnt;
    struct siwifi_sta *users[CONFIG_USER_MAX];
};
struct siwifi_mu_info {
    struct list_head active_groups;
    struct list_head active_sta;
    struct list_head update_sta;
    struct siwifi_mu_group groups[NX_MU_GROUP_MAX];
    struct delayed_work group_work;
    u16 update_count;
    struct semaphore lock;
    unsigned long next_group_select;
    u8 group_cnt;
};
#define SIWIFI_SU_GROUP BIT_ULL(0)
#define SIWIFI_MU_GROUP_MASK 0x7ffffffffffffffeULL
#define SIWIFI_MU_GROUP_INTERVAL 200
#define SIWIFI_MU_GROUP_SELECT_INTERVAL 100
#define SIWIFI_MU_GROUP_MIN_TRAFFIC 50
#define SIWIFI_GET_FIRST_GROUP_ID(map) (fls64(map) - 1)
#define group_sta_for_each(sta,id,map) \
    map = sta->group_info.map & SIWIFI_MU_GROUP_MASK; \
    for (id = (fls64(map) - 1) ; id > 0 ; \
         map &= ~(u64)BIT_ULL(id), id = (fls64(map) - 1))
#define group_for_each(id,map) \
    for (id = (fls64(map) - 1) ; id > 0 ; \
         map &= ~(u64)BIT_ULL(id), id = (fls64(map) - 1))
#define SIWIFI_MUMIMO_INFO_POS_ID(info) (((info) >> 6) & 0x3)
#define SIWIFI_MUMIMO_INFO_GROUP_ID(info) ((info) & 0x3f)
static inline
struct siwifi_mu_group *siwifi_mu_group_from_id(struct siwifi_mu_info *mu, int id)
{
    if (id > NX_MU_GROUP_MAX)
        return NULL;
    return &mu->groups[id - 1];
}
void siwifi_mu_group_sta_init(struct siwifi_sta *sta,
                            const struct ieee80211_vht_cap *vht_cap);
void siwifi_mu_group_sta_del(struct siwifi_hw *siwifi_hw, struct siwifi_sta *sta);
u64 siwifi_mu_group_sta_get_map(struct siwifi_sta *sta);
int siwifi_mu_group_sta_get_pos(struct siwifi_hw *siwifi_hw, struct siwifi_sta *sta,
                              int group_id);
void siwifi_mu_group_init(struct siwifi_hw *siwifi_hw);
void siwifi_mu_set_active_sta(struct siwifi_hw *siwifi_hw, struct siwifi_sta *sta,
                            int traffic);
void siwifi_mu_set_active_group(struct siwifi_hw *siwifi_hw, int group_id);
void siwifi_mu_group_sta_select(struct siwifi_hw *siwifi_hw);
#else
static inline
void siwifi_mu_group_sta_init(struct siwifi_sta *sta,
                            const struct ieee80211_vht_cap *vht_cap)
{}
static inline
void siwifi_mu_group_sta_del(struct siwifi_hw *siwifi_hw, struct siwifi_sta *sta)
{}
static inline
u64 siwifi_mu_group_sta_get_map(struct siwifi_sta *sta)
{
    return 0;
}
static inline
int siwifi_mu_group_sta_get_pos(struct siwifi_hw *siwifi_hw, struct siwifi_sta *sta,
                              int group_id)
{
    return 0;
}
static inline
void siwifi_mu_group_init(struct siwifi_hw *siwifi_hw)
{}
static inline
void siwifi_mu_set_active_sta(struct siwifi_hw *siwifi_hw, struct siwifi_sta *sta,
                            int traffic)
{}
static inline
void siwifi_mu_set_active_group(struct siwifi_hw *siwifi_hw, int group_id)
{}
static inline
void siwifi_mu_group_sta_select(struct siwifi_hw *siwifi_hw)
{}
#endif
#endif
