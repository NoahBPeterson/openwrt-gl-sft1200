#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <net/mac80211.h>
#include "siwifi_radar.h"
#include "siwifi_defs.h"
#include "siwifi_msg_tx.h"
#include "siwifi_events.h"
#include "siwifi_mem.h"
#define RADAR_MIN_REP 200
#define PRI_TOLERANCE 16
int freq_delta = 20;
struct radar_types {
    enum nl80211_dfs_regions region;
    u32 num_radar_types;
    const struct radar_detector_specs *spec_riu;
    const struct radar_detector_specs *spec_fcu;
};
enum radar_waveform_type {
    RADAR_WAVEFORM_SHORT,
    RADAR_WAVEFORM_WEATHER,
    RADAR_WAVEFORM_INTERLEAVED,
    RADAR_WAVEFORM_LONG
};
struct radar_detector_specs {
    u8 type_id;
    u8 width_min;
    u8 width_max;
    u16 pri_min;
    u16 pri_max;
    u8 num_pri;
    u8 ppb;
    u8 ppb_thresh;
    u8 max_pri_tolerance;
    enum radar_waveform_type type;
};
#define MIN_PPB_THRESH 50
#define PPB_THRESH(PPB) ((PPB * MIN_PPB_THRESH + 50) / 100)
#define PRF2PRI(PRF) ((1000000 + PRF / 2) / PRF)
#define WIDTH_TOLERANCE 2
#define WIDTH_LOWER(X) (X)
#define WIDTH_UPPER(X) (X)
#define ETSI_PATTERN_SHORT(ID,WMIN,WMAX,PMIN,PMAX,PPB) \
    { \
        ID, WIDTH_LOWER(WMIN), WIDTH_UPPER(WMAX), \
            (PRF2PRI(PMAX) - PRI_TOLERANCE), \
            (PRF2PRI(PMIN) + PRI_TOLERANCE), 1, PPB, \
            PPB_THRESH(PPB), PRI_TOLERANCE, RADAR_WAVEFORM_SHORT \
            }
#define ETSI_PATTERN_INTERLEAVED(ID,WMIN,WMAX,PMIN,PMAX,PRFMIN,PRFMAX,PPB) \
    { \
        ID, WIDTH_LOWER(WMIN), WIDTH_UPPER(WMAX), \
            (PRF2PRI(PMAX) * PRFMIN- PRI_TOLERANCE), \
            (PRF2PRI(PMIN) * PRFMAX + PRI_TOLERANCE), \
            PRFMAX, PPB * PRFMAX, \
            PPB_THRESH(PPB), PRI_TOLERANCE, RADAR_WAVEFORM_INTERLEAVED \
            }
static const struct radar_detector_specs etsi_radar_ref_types_v17_riu[] = {
    ETSI_PATTERN_SHORT(0, 0, 8, 700, 700, 18),
    ETSI_PATTERN_SHORT(1, 0, 10, 200, 1000, 10),
    ETSI_PATTERN_SHORT(2, 0, 22, 200, 1600, 15),
    ETSI_PATTERN_SHORT(3, 0, 22, 2300, 4000, 25),
    ETSI_PATTERN_SHORT(4, 20, 38, 2000, 4000, 20),
    ETSI_PATTERN_INTERLEAVED(5, 0, 8, 300, 400, 2, 3, 10),
    ETSI_PATTERN_INTERLEAVED(6, 0, 8, 400, 1200, 2, 3, 15),
};
static const struct radar_detector_specs etsi_radar_ref_types_v17_fcu[] = {
    ETSI_PATTERN_SHORT(0, 0, 8, 700, 700, 18),
    ETSI_PATTERN_SHORT(1, 0, 8, 200, 1000, 10),
    ETSI_PATTERN_SHORT(2, 0, 16, 200, 1600, 15),
    ETSI_PATTERN_SHORT(3, 0, 16, 2300, 4000, 25),
    ETSI_PATTERN_SHORT(4, 20, 34, 2000, 4000, 20),
    ETSI_PATTERN_INTERLEAVED(5, 0, 8, 300, 400, 2, 3, 10),
    ETSI_PATTERN_INTERLEAVED(6, 0, 8, 400, 1200, 2, 3, 15),
};
static const struct radar_types etsi_radar_types_v17 = {
    .region = NL80211_DFS_ETSI,
    .num_radar_types = ARRAY_SIZE(etsi_radar_ref_types_v17_riu),
    .spec_riu = etsi_radar_ref_types_v17_riu,
    .spec_fcu = etsi_radar_ref_types_v17_fcu,
};
#define FCC_PATTERN(ID,WMIN,WMAX,PMIN,PMAX,PRF,PPB,TYPE) \
    { \
        ID, WIDTH_LOWER(WMIN), WIDTH_UPPER(WMAX), \
            PMIN - PRI_TOLERANCE, \
            PMAX * PRF + PRI_TOLERANCE, PRF, PPB * PRF, \
            PPB_THRESH(PPB), PRI_TOLERANCE, TYPE \
            }
static const struct radar_detector_specs fcc_radar_ref_types_riu[] = {
    FCC_PATTERN(0, 0, 8, 1428, 1428, 1, 18, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(1, 0, 8, 518, 3066, 1, 102, RADAR_WAVEFORM_WEATHER),
    FCC_PATTERN(2, 0, 8, 150, 230, 1, 23, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(3, 6, 20, 200, 500, 1, 16, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(4, 10, 28, 200, 500, 1, 12, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(5, 50, 110, 1000, 2000, 1, 8, RADAR_WAVEFORM_LONG),
    FCC_PATTERN(6, 0, 8, 333, 333, 1, 9, RADAR_WAVEFORM_SHORT),
};
static const struct radar_detector_specs fcc_radar_ref_types_fcu[] = {
    FCC_PATTERN(0, 0, 8, 1428, 1428, 1, 18, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(1, 0, 8, 518, 3066, 1, 102, RADAR_WAVEFORM_WEATHER),
    FCC_PATTERN(2, 0, 8, 150, 230, 1, 23, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(3, 6, 12, 200, 500, 1, 16, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(4, 10, 22, 200, 500, 1, 12, RADAR_WAVEFORM_SHORT),
    FCC_PATTERN(5, 50, 104, 1000, 2000, 1, 8, RADAR_WAVEFORM_LONG),
    FCC_PATTERN(6, 0, 8, 333, 333, 1, 9, RADAR_WAVEFORM_SHORT),
};
static const struct radar_types fcc_radar_types = {
    .region = NL80211_DFS_FCC,
    .num_radar_types = ARRAY_SIZE(fcc_radar_ref_types_riu),
    .spec_riu = fcc_radar_ref_types_riu,
    .spec_fcu = fcc_radar_ref_types_fcu,
};
#define JP_PATTERN FCC_PATTERN
static const struct radar_detector_specs jp_radar_ref_types_riu[] = {
    JP_PATTERN(0, 0, 8, 1428, 1428, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(1, 2, 8, 3846, 3846, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(2, 0, 8, 1388, 1388, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(3, 0, 8, 4000, 4000, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(4, 0, 8, 150, 230, 1, 23, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(5, 6, 20, 200, 500, 1, 16, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(6, 10, 28, 200, 500, 1, 12, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(7, 50, 110, 1000, 2000, 1, 8, RADAR_WAVEFORM_LONG),
    JP_PATTERN(8, 0, 8, 333, 333, 1, 9, RADAR_WAVEFORM_SHORT),
};
static const struct radar_detector_specs jp_radar_ref_types_fcu[] = {
    JP_PATTERN(0, 0, 8, 1428, 1428, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(1, 2, 6, 3846, 3846, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(2, 0, 8, 1388, 1388, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(3, 2, 2, 4000, 4000, 1, 18, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(4, 0, 8, 150, 230, 1, 23, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(5, 6, 12, 200, 500, 1, 16, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(6, 10, 22, 200, 500, 1, 12, RADAR_WAVEFORM_SHORT),
    JP_PATTERN(7, 50, 104, 1000, 2000, 1, 8, RADAR_WAVEFORM_LONG),
    JP_PATTERN(8, 0, 8, 333, 333, 1, 9, RADAR_WAVEFORM_SHORT),
};
static const struct radar_types jp_radar_types = {
    .region = NL80211_DFS_JP,
    .num_radar_types = ARRAY_SIZE(jp_radar_ref_types_riu),
    .spec_riu = jp_radar_ref_types_riu,
    .spec_fcu = jp_radar_ref_types_fcu,
};
static const struct radar_types *dfs_domains[] = {
    &etsi_radar_types_v17,
    &fcc_radar_types,
    &jp_radar_types,
};
struct pri_sequence {
    struct list_head head;
    u32 pri;
    u32 dur;
    u32 count;
    u32 count_falses;
    u64 first_ts;
    u64 last_ts;
    u64 deadline_ts;
    u8 ppb_thresh;
};
struct pulse_elem {
    struct list_head head;
    u64 ts;
};
struct pri_detector {
    struct list_head head;
    const struct radar_detector_specs *rs;
    u64 last_ts;
    struct list_head sequences;
    struct list_head pulses;
    u32 count;
    u32 max_count;
    u32 window_size;
    struct pri_detector_ops *ops;
    u16 freq;
};
struct pri_detector_ops {
    void (*init)(struct pri_detector *pde);
    struct pri_sequence * (*add_pulse)(struct pri_detector *pde, u16 len, u64 ts, u16 pri);
    int reset_on_pri_overflow;
};
static u32 singleton_pool_references;
static LIST_HEAD(pulse_pool);
static LIST_HEAD(pseq_pool);
static DEFINE_SPINLOCK(pool_lock);
static void pool_register_ref(void)
{
    spin_lock_bh(&pool_lock);
    singleton_pool_references++;
    spin_unlock_bh(&pool_lock);
}
static void pool_deregister_ref(void)
{
    spin_lock_bh(&pool_lock);
    singleton_pool_references--;
    if (singleton_pool_references == 0) {
        struct pri_sequence *ps, *ps0;
        struct pulse_elem *p, *p0;
        list_for_each_entry_safe(p, p0, &pulse_pool, head) {
            list_del(&p->head);
            siwifi_kfree(p);
        }
        list_for_each_entry_safe(ps, ps0, &pseq_pool, head) {
            list_del(&ps->head);
            siwifi_kfree(ps);
        }
    }
    spin_unlock_bh(&pool_lock);
}
static void pool_put_pulse_elem(struct pulse_elem *pe)
{
    spin_lock_bh(&pool_lock);
    list_add(&pe->head, &pulse_pool);
    spin_unlock_bh(&pool_lock);
}
static void pool_put_pseq_elem(struct pri_sequence *pse)
{
    spin_lock_bh(&pool_lock);
    list_add(&pse->head, &pseq_pool);
    spin_unlock_bh(&pool_lock);
}
static struct pri_sequence *pool_get_pseq_elem(void)
{
    struct pri_sequence *pse = NULL;
    spin_lock_bh(&pool_lock);
    if (!list_empty(&pseq_pool)) {
        pse = list_first_entry(&pseq_pool, struct pri_sequence, head);
        list_del(&pse->head);
    }
    spin_unlock_bh(&pool_lock);
    if (pse == NULL) {
        pse = siwifi_kmalloc(sizeof(*pse), GFP_ATOMIC);
    }
    return pse;
}
static struct pulse_elem *pool_get_pulse_elem(void)
{
    struct pulse_elem *pe = NULL;
    spin_lock_bh(&pool_lock);
    if (!list_empty(&pulse_pool)) {
        pe = list_first_entry(&pulse_pool, struct pulse_elem, head);
        list_del(&pe->head);
    }
    spin_unlock_bh(&pool_lock);
    return pe;
}
static struct pulse_elem *pulse_queue_get_tail(struct pri_detector *pde)
{
    struct list_head *l = &pde->pulses;
    if (list_empty(l))
        return NULL;
    return list_entry(l->prev, struct pulse_elem, head);
}
static bool pulse_queue_dequeue(struct pri_detector *pde)
{
    struct pulse_elem *p = pulse_queue_get_tail(pde);
    if (p != NULL) {
        list_del_init(&p->head);
        pde->count--;
        pool_put_pulse_elem(p);
    }
    return (pde->count > 0);
}
static
void pulse_queue_check_window(struct pri_detector *pde)
{
    u64 min_valid_ts;
    struct pulse_elem *p;
    if (pde->count < 2)
        return;
    if (pde->last_ts <= pde->window_size)
        return;
    min_valid_ts = pde->last_ts - pde->window_size;
    while ((p = pulse_queue_get_tail(pde)) != NULL) {
        if (p->ts >= min_valid_ts)
            return;
        pulse_queue_dequeue(pde);
    }
}
static
bool pulse_queue_enqueue(struct pri_detector *pde, u64 ts)
{
    struct pulse_elem *p = pool_get_pulse_elem();
    if (p == NULL) {
        p = siwifi_kmalloc(sizeof(*p), GFP_ATOMIC);
        if (p == NULL) {
             return false;
        }
    }
    INIT_LIST_HEAD(&p->head);
    p->ts = ts;
    list_add(&p->head, &pde->pulses);
    pde->count++;
    pde->last_ts = ts;
    pulse_queue_check_window(pde);
    if (pde->count >= pde->max_count)
        pulse_queue_dequeue(pde);
    return true;
}
static
u32 pde_get_multiple(u32 val, u32 fraction, u32 tolerance)
{
    u32 remainder;
    u32 factor;
    u32 delta;
    if (fraction == 0)
        return 0;
    delta = (val < fraction) ? (fraction - val) : (val - fraction);
    if (delta <= tolerance)
        return 1;
    factor = val / fraction;
    remainder = val % fraction;
    if (remainder > tolerance) {
        if ((fraction - remainder) <= tolerance)
            factor++;
        else
            factor = 0;
    }
    return factor;
}
static
bool pde_short_create_sequences(struct pri_detector *pde,
                                u64 ts, u32 min_count)
{
    struct pulse_elem *p;
    u16 pulse_idx = 0;
    list_for_each_entry(p, &pde->pulses, head) {
        struct pri_sequence ps, *new_ps;
        struct pulse_elem *p2;
        u32 tmp_false_count;
        u64 min_valid_ts;
        u32 delta_ts = ts - p->ts;
        pulse_idx++;
        if (delta_ts < pde->rs->pri_min)
            continue;
        if (delta_ts > pde->rs->pri_max)
            break;
        ps.count = 2;
        ps.count_falses = 0;
        ps.first_ts = p->ts;
        ps.last_ts = ts;
        ps.pri = ts - p->ts;
        ps.dur = ps.pri * (pde->rs->ppb - 1)
            + 2 * pde->rs->max_pri_tolerance;
        p2 = p;
        tmp_false_count = 0;
        if (ps.dur > ts)
            min_valid_ts = 0;
        else
            min_valid_ts = ts - ps.dur;
        list_for_each_entry_continue(p2, &pde->pulses, head) {
            u32 factor;
            if (p2->ts < min_valid_ts)
                break;
            factor = pde_get_multiple(ps.last_ts - p2->ts, ps.pri,
                                      pde->rs->max_pri_tolerance);
            if (factor > 0) {
                ps.count++;
                ps.first_ts = p2->ts;
                ps.count_falses += tmp_false_count;
                tmp_false_count = 0;
            } else {
                tmp_false_count++;
            }
        }
        if (ps.count <= min_count) {
            continue;
        }
        ps.deadline_ts = ps.first_ts + ps.dur;
        if (pde->rs->type == RADAR_WAVEFORM_WEATHER) {
            ps.ppb_thresh = 19000000 / (360 * ps.pri);
            ps.ppb_thresh = PPB_THRESH(ps.ppb_thresh);
        } else {
            ps.ppb_thresh = pde->rs->ppb_thresh;
        }
        new_ps = pool_get_pseq_elem();
        if (new_ps == NULL) {
            return false;
        }
        memcpy(new_ps, &ps, sizeof(ps));
        INIT_LIST_HEAD(&new_ps->head);
        list_add(&new_ps->head, &pde->sequences);
    }
    return true;
}
static
u32 pde_short_add_to_existing_seqs(struct pri_detector *pde, u64 ts)
{
    u32 max_count = 0;
    struct pri_sequence *ps, *ps2;
    list_for_each_entry_safe(ps, ps2, &pde->sequences, head) {
        u32 delta_ts;
        u32 factor;
        if (ts > ps->deadline_ts) {
            list_del_init(&ps->head);
            pool_put_pseq_elem(ps);
            continue;
        }
        delta_ts = ts - ps->last_ts;
        factor = pde_get_multiple(delta_ts, ps->pri,
                                  pde->rs->max_pri_tolerance);
        if (factor > 0) {
            ps->last_ts = ts;
            ps->count++;
            if (max_count < ps->count)
                max_count = ps->count;
        } else {
            ps->count_falses++;
        }
    }
    return max_count;
}
static
struct pri_sequence * pde_short_check_detection(struct pri_detector *pde)
{
    struct pri_sequence *ps;
    if (list_empty(&pde->sequences))
        return NULL;
    list_for_each_entry(ps, &pde->sequences, head) {
        if ((ps->count >= ps->ppb_thresh) &&
            (ps->count * pde->rs->num_pri > ps->count_falses)) {
            return ps;
        }
    }
    return NULL;
}
static
void pde_short_init(struct pri_detector *pde)
{
    pde->window_size = pde->rs->pri_max * pde->rs->ppb * pde->rs->num_pri;
    pde->max_count = pde->rs->ppb * 2;
}
static void pri_detector_reset(struct pri_detector *pde, u64 ts);
static
struct pri_sequence *pde_short_add_pulse(struct pri_detector *pde,
                                         u16 len, u64 ts, u16 pri)
{
    u32 max_updated_seq;
    struct pri_sequence *ps;
    const struct radar_detector_specs *rs = pde->rs;
    if (pde->count == 0) {
        pulse_queue_enqueue(pde, ts);
        return NULL;
    }
    if ((ts - pde->last_ts) < rs->max_pri_tolerance) {
        return NULL;
    }
    max_updated_seq = pde_short_add_to_existing_seqs(pde, ts);
    if (!pde_short_create_sequences(pde, ts, max_updated_seq)) {
        pri_detector_reset(pde, ts);
        return NULL;
    }
    ps = pde_short_check_detection(pde);
    if (ps == NULL)
        pulse_queue_enqueue(pde, ts);
    return ps;
}
static struct pri_detector_ops pri_detector_short = {
    .init = pde_short_init,
    .add_pulse = pde_short_add_pulse,
    .reset_on_pri_overflow = 1,
};
#define LONG_RADAR_DURATION 12000000
#define LONG_RADAR_BURST_MIN_DURATION (12000000 / 20)
#define LONG_RADAR_MAX_BURST 20
static
void pde_long_init(struct pri_detector *pde)
{
    pde->window_size = LONG_RADAR_DURATION;
    pde->max_count = LONG_RADAR_MAX_BURST;
}
static
struct pri_sequence *pde_long_add_pulse(struct pri_detector *pde,
                                        u16 len, u64 ts, u16 pri)
{
    struct pri_sequence *ps;
    const struct radar_detector_specs *rs = pde->rs;
    if (list_empty(&pde->sequences)) {
        ps = pool_get_pseq_elem();
        if (ps == NULL) {
            return NULL;
        }
        ps->count = 1;
        ps->count_falses = 1;
        ps->first_ts = ts;
        ps->last_ts = ts;
        ps->deadline_ts = ts + pde->window_size;
        ps->pri = 0;
        INIT_LIST_HEAD(&ps->head);
        list_add(&ps->head, &pde->sequences);
        pulse_queue_enqueue(pde, ts);
    } else {
        u32 delta_ts;
        ps = (struct pri_sequence *)pde->sequences.next;
        delta_ts = ts - ps->last_ts;
        ps->last_ts = ts;
        if (delta_ts < rs->pri_max) {
        } else if ((delta_ts >= rs->pri_min) &&
              (delta_ts <= rs->pri_max)) {
            ps->count_falses++;
        } else if ((ps->count > 2) &&
                   (ps->dur + delta_ts) < LONG_RADAR_BURST_MIN_DURATION) {
        } else {
            ps->count++;
            ps->count_falses = 1;
            if (ts > ps->deadline_ts) {
                struct pulse_elem *p;
                u64 min_valid_ts;
                min_valid_ts = ts - pde->window_size;
                while ((p = pulse_queue_get_tail(pde)) != NULL) {
                    if (p->ts >= min_valid_ts) {
                        ps->first_ts = p->ts;
                        ps->deadline_ts = p->ts + pde->window_size;
                        break;
                    }
                    pulse_queue_dequeue(pde);
                    ps->count--;
                }
            }
            if (ps->count > pde->rs->ppb_thresh &&
                (ts - ps->first_ts) > (pde->window_size / 2)) {
                return ps;
            } else {
                pulse_queue_enqueue(pde, ts);
                ps->dur = delta_ts;
            }
        }
    }
    return NULL;
}
static struct pri_detector_ops pri_detector_long = {
    .init = pde_long_init,
    .add_pulse = pde_long_add_pulse,
    .reset_on_pri_overflow = 0,
};
struct pri_detector *pri_detector_init(struct dfs_pattern_detector *dpd,
                                       u16 radar_type, u16 freq)
{
    struct pri_detector *pde;
    pde = siwifi_kzalloc(sizeof(*pde), GFP_ATOMIC);
    if (pde == NULL)
        return NULL;
    INIT_LIST_HEAD(&pde->sequences);
    INIT_LIST_HEAD(&pde->pulses);
    INIT_LIST_HEAD(&pde->head);
    list_add(&pde->head, &dpd->detectors[radar_type]);
    pde->rs = &dpd->radar_spec[radar_type];
    pde->freq = freq;
    if (pde->rs->type == RADAR_WAVEFORM_LONG) {
        pde->ops = &pri_detector_long;
    } else {
        pde->ops = &pri_detector_short;
    }
    pde->ops->init(pde);
    pool_register_ref();
    return pde;
}
static
void pri_detector_reset(struct pri_detector *pde, u64 ts)
{
    struct pri_sequence *ps, *ps0;
    struct pulse_elem *p, *p0;
    list_for_each_entry_safe(ps, ps0, &pde->sequences, head) {
        list_del_init(&ps->head);
        pool_put_pseq_elem(ps);
    }
    list_for_each_entry_safe(p, p0, &pde->pulses, head) {
        list_del_init(&p->head);
        pool_put_pulse_elem(p);
    }
    pde->count = 0;
    pde->last_ts = ts;
}
static
void pri_detector_exit(struct pri_detector *pde)
{
    pri_detector_reset(pde, 0);
    pool_deregister_ref();
    list_del(&pde->head);
    siwifi_kfree(pde);
}
static struct pri_detector *
pri_detector_get(struct dfs_pattern_detector *dpd, u16 freq, u16 radar_type)
{
    struct pri_detector *pde, *cur = NULL;
    int max_count = 0;
    list_for_each_entry(pde, &dpd->detectors[radar_type], head) {
#if 0
        if (pde->freq == freq) {
            if (pde->count)
                return pde;
            else
                cur = pde;
        } else if (pde->freq - 2 == freq && pde->count) {
            return pde;
        } else if (pde->freq + 2 == freq && pde->count) {
            return pde;
        }
#else
        if ((pde->freq - freq_delta < freq) && (pde->freq + freq_delta > freq) && (pde->count >= max_count)) {
    max_count = pde->count;
    cur = pde;
  }
#endif
    }
    if (cur)
        return cur;
    else
        return pri_detector_init(dpd, radar_type, freq);
}
static void dfs_pattern_detector_reset(struct dfs_pattern_detector *dpd)
{
    struct pri_detector *pde;
    int i;
    for (i = 0; i < dpd->num_radar_types; i++) {
        if (!list_empty(&dpd->detectors[i]))
            list_for_each_entry(pde, &dpd->detectors[i], head)
                pri_detector_reset(pde, dpd->last_pulse_ts);
    }
    dpd->last_pulse_ts = 0;
    dpd->prev_jiffies = jiffies;
}
static void dfs_pattern_detector_exit(struct dfs_pattern_detector *dpd)
{
    struct pri_detector *pde, *pde0;
    int i;
    for (i = 0; i < dpd->num_radar_types; i++) {
        if (!list_empty(&dpd->detectors[i]))
            list_for_each_entry_safe(pde, pde0, &dpd->detectors[i], head)
                pri_detector_exit(pde);
    }
    siwifi_kfree(dpd);
}
static void dfs_pattern_detector_pri_overflow(struct dfs_pattern_detector *dpd)
{
    struct pri_detector *pde;
    int i;
    for (i = 0; i < dpd->num_radar_types; i++) {
        if (!list_empty(&dpd->detectors[i]))
            list_for_each_entry(pde, &dpd->detectors[i], head)
                if (pde->ops->reset_on_pri_overflow)
                    pri_detector_reset(pde, dpd->last_pulse_ts);
    }
}
static bool dfs_pattern_detector_add_pulse(struct dfs_pattern_detector *dpd,
                                           enum siwifi_radar_chain chain,
                                           u16 freq, u16 pri, u16 len, u32 now)
{
    u32 i;
    u32 delta_jiffie;
    if (dpd->region == NL80211_DFS_UNSET)
        return true;
    if ((pri == 0) || (dpd->consecutive_too_short_count >= 2)) {
        if (unlikely(now < dpd->prev_jiffies)) {
            delta_jiffie = 0xffffffff - dpd->prev_jiffies + now;
        } else {
            delta_jiffie = now - dpd->prev_jiffies;
        }
        dpd->last_pulse_ts += jiffies_to_usecs(delta_jiffie);
        dpd->prev_jiffies = now;
        dfs_pattern_detector_pri_overflow(dpd);
        dpd->total_pulses.index = 0;
        dpd->total_pulses.count = 0;
        dpd->consecutive_too_short_count = 0;
    } else {
        dpd->last_pulse_ts += pri;
    }
    for (i = 0; i < dpd->num_radar_types; i++) {
        struct pri_sequence *ps;
        struct pri_detector *pde;
        const struct radar_detector_specs *rs = &dpd->radar_spec[i];
        if ((rs->width_min > len) ||
            (rs->width_max < len)) {
            continue;
        }
        pde = pri_detector_get(dpd, freq, i);
        ps = pde->ops->add_pulse(pde, len, dpd->last_pulse_ts, pri);
        if (ps != NULL) {
            trace_radar_detected(chain, dpd->region, pde->freq, i, ps->pri);
            printk("radar detected chain %d dpd region %d freq %u radar type %d pri %u count %u num_pri %u thresh %u falses count %u\n",
                    chain, dpd->region, pde->freq, i, ps->pri, ps->count, pde->rs->num_pri, ps->ppb_thresh, ps->count_falses);
            dfs_pattern_detector_reset(dpd);
            return true;
        }
    }
    return false;
}
static const struct radar_types *
get_dfs_domain_radar_types(enum nl80211_dfs_regions region)
{
    u32 i;
    for (i = 0; i < ARRAY_SIZE(dfs_domains); i++) {
        if (dfs_domains[i]->region == region)
            return dfs_domains[i];
    }
    return NULL;
}
static u16 get_dfs_max_radar_types(void)
{
    u32 i;
    u16 max = 0;
    for (i = 0; i < ARRAY_SIZE(dfs_domains); i++) {
        if (dfs_domains[i]->num_radar_types > max)
            max = dfs_domains[i]->num_radar_types;
    }
    return max;
}
static
bool dfs_pattern_detector_set_domain(struct dfs_pattern_detector *dpd,
                                     enum nl80211_dfs_regions region, u8 chain)
{
    const struct radar_types *rt;
    struct pri_detector *pde, *pde0;
    int i;
    if (dpd->region == region)
        return true;
    dpd->region = NL80211_DFS_UNSET;
    rt = get_dfs_domain_radar_types(region);
    if (rt == NULL)
        return false;
    for (i = 0; i < dpd->num_radar_types; i++) {
        if (!list_empty(&dpd->detectors[i]))
            list_for_each_entry_safe(pde, pde0, &dpd->detectors[i], head)
                pri_detector_exit(pde);
    }
    if (chain == SIWIFI_RADAR_RIU)
        dpd->radar_spec = rt->spec_riu;
    else
        dpd->radar_spec = rt->spec_fcu;
    dpd->num_radar_types = rt->num_radar_types;
    dpd->region = region;
    return true;
}
static struct dfs_pattern_detector *
dfs_pattern_detector_init(enum nl80211_dfs_regions region, u8 chain)
{
    struct dfs_pattern_detector *dpd;
    u16 i, max_radar_type = get_dfs_max_radar_types();
    dpd = siwifi_kmalloc(sizeof(*dpd) + max_radar_type * sizeof(dpd->detectors[0]),
                  GFP_KERNEL);
    if (dpd == NULL)
        return NULL;
    dpd->region = NL80211_DFS_UNSET;
    dpd->enabled = SIWIFI_RADAR_DETECT_DISABLE;
    dpd->last_pulse_ts = 0;
    dpd->prev_jiffies = jiffies;
    dpd->num_radar_types = 0;
    for (i = 0; i < max_radar_type; i++)
        INIT_LIST_HEAD(&dpd->detectors[i]);
    if (dfs_pattern_detector_set_domain(dpd, region, chain))
        return dpd;
    siwifi_kfree(dpd);
    return NULL;
}
static u16 siwifi_radar_get_center_freq(struct siwifi_hw *siwifi_hw, u8 chain)
{
    if (chain == SIWIFI_RADAR_FCU)
        return siwifi_hw->sec_phy_chan.center_freq1;
    if (chain == SIWIFI_RADAR_RIU) {
        spin_lock_bh(&siwifi_hw->cb_lock);
        if (!siwifi_chanctx_valid(siwifi_hw, siwifi_hw->cur_chanctx)) {
            printk("Radar pulse without channel information\n");
        } else {
            spin_unlock_bh(&siwifi_hw->cb_lock);
            return siwifi_hw->chanctx_table[siwifi_hw->cur_chanctx].chan_def.center_freq1;
        }
        spin_unlock_bh(&siwifi_hw->cb_lock);
    }
    return 0;
}
void siwifi_radar_detected(struct siwifi_hw *siwifi_hw)
{
    struct cfg80211_chan_def chan_def;
    spin_lock_bh(&siwifi_hw->cb_lock);
    if (!siwifi_chanctx_valid(siwifi_hw, siwifi_hw->cur_chanctx)) {
        spin_unlock_bh(&siwifi_hw->cb_lock);
        WARN(1, "Radar detected without channel information");
        return;
    }
#if DEBUG_ARRAY_CHECK
    BUG_ON(siwifi_hw->cur_chanctx >= NX_CHAN_CTXT_CNT);
#endif
    chan_def = siwifi_hw->chanctx_table[siwifi_hw->cur_chanctx].chan_def;
    siwifi_radar_cancel_cac(&siwifi_hw->radar);
    spin_unlock_bh(&siwifi_hw->cb_lock);
    cfg80211_radar_event(siwifi_hw->wiphy, &chan_def, GFP_KERNEL);
}
static void printk_detected_pulses(struct dfs_pattern_detector *dpd)
{
    int i;
    struct radar_pulse *p = NULL;
    if (dpd->total_pulses.count > 32) {
        for (i = dpd->total_pulses.index; i < 32; i++)
        {
            p = (struct radar_pulse *)&dpd->total_pulses.buffer[i];
            printk("pulse freq %d fom %d len %d rep %d\n", p->freq, p->fom, p->len, p->rep);
        }
    }
    for (i = 0; i < dpd->total_pulses.index; i++)
    {
        p = (struct radar_pulse *)&dpd->total_pulses.buffer[i];
        printk("pulse freq %d fom %d len %d rep %d\n", p->freq, p->fom, p->len, p->rep);
    }
}
static void siwifi_radar_process_pulse(struct work_struct *ws)
{
    struct siwifi_radar *radar = container_of(ws, struct siwifi_radar,
                                            detection_work);
    struct siwifi_hw *siwifi_hw = container_of(radar, struct siwifi_hw, radar);
    int chain;
    u32 pulses[SIWIFI_RADAR_LAST][SIWIFI_RADAR_PULSE_MAX] = { 0 };
    u16 pulses_count[SIWIFI_RADAR_LAST];
    u32 now = jiffies;
    struct siwifi_vif *siwifi_vif;
    int wds_mode = 0;
    spin_lock_bh(&radar->lock);
    for (chain = SIWIFI_RADAR_RIU; chain < SIWIFI_RADAR_LAST; chain++) {
        int start, count;
        count = radar->pulses[chain].count;
        start = radar->pulses[chain].index - count;
        if (start < 0)
            start += SIWIFI_RADAR_PULSE_MAX;
        pulses_count[chain] = count;
        if (count == 0)
            continue;
        if ((start + count) > SIWIFI_RADAR_PULSE_MAX) {
            u16 count1 = (SIWIFI_RADAR_PULSE_MAX - start);
            memcpy(&(pulses[chain][0]),
                   &(radar->pulses[chain].buffer[start]),
                   count1 * sizeof(struct radar_pulse));
            memcpy(&(pulses[chain][count1]),
                   &(radar->pulses[chain].buffer[0]),
                   (count - count1) * sizeof(struct radar_pulse));
        } else {
            memcpy(&(pulses[chain][0]),
                   &(radar->pulses[chain].buffer[start]),
                   count * sizeof(struct radar_pulse));
        }
        radar->pulses[chain].count = 0;
    }
    spin_unlock_bh(&radar->lock);
    for (chain = SIWIFI_RADAR_RIU; chain < SIWIFI_RADAR_LAST; chain++) {
        int i;
        u16 freq;
        struct timespec64 ts;
        if (pulses_count[chain] == 0)
            continue;
        freq = siwifi_radar_get_center_freq(siwifi_hw, chain);
        if (!freq) {
            printk("current freq is 0, skip radar detected \n");
            break;
        }
        for (i = 0; i < pulses_count[chain] ; i++) {
            struct radar_pulse *p = (struct radar_pulse *)&pulses[chain][i];
            if (!p){
                continue;
            }
            if (radar->dpd[chain]->total_pulses.index < 0 || radar->dpd[chain]->total_pulses.index > SIWIFI_RADAR_PULSE_MAX)
            {
                printk("radar->dpd[chain]->total_pulses.index(%d) is err. Make it equal to 0", radar->dpd[chain]->total_pulses.index);
                radar->dpd[chain]->total_pulses.index = 0;
            }
            trace_radar_pulse(chain, p);
            if(siwifi_hw->mod_params->radar_debug_printk)
                printk("radar chain %d pulse freq %d fom %d len %d rep %d\n", chain, p->freq, p->fom, p->len, p->rep);
            memcpy(&(radar->dpd[chain]->total_pulses.buffer[radar->dpd[chain]->total_pulses.index]), p,
                   sizeof(struct radar_pulse));
            radar->dpd[chain]->total_pulses.index ++;
            if(radar->dpd[chain]->total_pulses.index == SIWIFI_RADAR_PULSE_MAX)
                radar->dpd[chain]->total_pulses.index = 0;
            radar->dpd[chain]->total_pulses.count ++;
            if (p->rep < RADAR_MIN_REP)
                radar->dpd[chain]->consecutive_too_short_count ++;
            else
                radar->dpd[chain]->consecutive_too_short_count = 0;
            if (dfs_pattern_detector_add_pulse(radar->dpd[chain], chain,
                                               (s16)freq + (2 * p->freq),
                                               p->rep, RADAR_LENGTH(p->len), now)) {
                u16 idx = radar->detected[chain].index;
                printk("radar detected success pulses count %d last 32 pulses: \n", radar->dpd[chain]->total_pulses.count);
                printk_detected_pulses(radar->dpd[chain]);
                if (chain == SIWIFI_RADAR_RIU) {
                    if (radar->dpd[chain]->enabled == SIWIFI_RADAR_DETECT_REPORT) {
                        list_for_each_entry(siwifi_vif, &siwifi_hw->vifs, list) {
                            if(SIWIFI_VIF_TYPE(siwifi_vif) == NL80211_IFTYPE_STATION && siwifi_vif->wds_success == 1){
                                wds_mode = 1;
                                break;
                            }
                            wds_mode = 0;
                        }
                        if(siwifi_hw->mod_params->radar_debugmode || wds_mode)
                        {
                            printk("radar detected but in radar debugmode or wdsmode, just print this msg,will not switch channel\n");
                            break;
                        }
                        siwifi_radar_detected(siwifi_hw);
                        siwifi_radar_detection_enable(radar,
                                                    SIWIFI_RADAR_DETECT_DISABLE,
                                                    chain);
                        radar->pulses[chain].count = 0;
                        radar->dpd[chain]->total_pulses.index = 0;
                        radar->dpd[chain]->total_pulses.count = 0;
                        radar->dpd[chain]->consecutive_too_short_count = 0;
                    }
                } else {
                }
                radar->detected[chain].freq[idx] = (s16)freq + (2 * p->freq);
                ktime_get_real_ts64(&ts);
                radar->detected[chain].time[idx] = ts.tv_sec;
                radar->detected[chain].index = ((idx + 1 ) %
                                                NX_NB_RADAR_DETECTED);
                radar->detected[chain].count++;
                break;
             }
        }
    }
}
static void siwifi_radar_cac_work(struct work_struct *ws)
{
    struct delayed_work *dw = container_of(ws, struct delayed_work, work);
    struct siwifi_radar *radar = container_of(dw, struct siwifi_radar, cac_work);
    struct siwifi_hw *siwifi_hw = container_of(radar, struct siwifi_hw, radar);
    struct siwifi_chanctx *ctxt;
    spin_lock_bh(&siwifi_hw->cb_lock);
    if (radar->cac_vif == NULL) {
        spin_unlock_bh(&siwifi_hw->cb_lock);
        WARN(1, "CAC finished but no vif set");
        return;
    }
    ctxt = &siwifi_hw->chanctx_table[radar->cac_vif->ch_index];
    cfg80211_cac_event(radar->cac_vif->ndev, &ctxt->chan_def,
                       NL80211_RADAR_CAC_FINISHED, GFP_KERNEL, 0);
 spin_unlock_bh(&siwifi_hw->cb_lock);
    siwifi_send_apm_stop_cac_req(siwifi_hw, radar->cac_vif);
 spin_lock_bh(&siwifi_hw->cb_lock);
    siwifi_chanctx_unlink(radar->cac_vif);
    radar->cac_vif = NULL;
    spin_unlock_bh(&siwifi_hw->cb_lock);
}
bool siwifi_radar_detection_init(struct siwifi_radar *radar)
{
    spin_lock_init(&radar->lock);
    radar->dpd[SIWIFI_RADAR_RIU] = dfs_pattern_detector_init(NL80211_DFS_UNSET,
                                                           SIWIFI_RADAR_RIU);
    if (radar->dpd[SIWIFI_RADAR_RIU] == NULL)
        return false;
    radar->dpd[SIWIFI_RADAR_FCU] = dfs_pattern_detector_init(NL80211_DFS_UNSET,
                                                           SIWIFI_RADAR_FCU);
    if (radar->dpd[SIWIFI_RADAR_FCU] == NULL) {
        siwifi_radar_detection_deinit(radar);
        return false;
    }
    INIT_WORK(&radar->detection_work, siwifi_radar_process_pulse);
    INIT_DELAYED_WORK(&radar->cac_work, siwifi_radar_cac_work);
    radar->cac_vif = NULL;
    return true;
}
void siwifi_radar_detection_deinit(struct siwifi_radar *radar)
{
    cancel_delayed_work_sync(&radar->cac_work);
    cancel_work_sync(&radar->detection_work);
    if (radar->dpd[SIWIFI_RADAR_RIU]) {
        dfs_pattern_detector_exit(radar->dpd[SIWIFI_RADAR_RIU]);
        radar->dpd[SIWIFI_RADAR_RIU] = NULL;
    }
    if (radar->dpd[SIWIFI_RADAR_FCU]) {
        dfs_pattern_detector_exit(radar->dpd[SIWIFI_RADAR_FCU]);
        radar->dpd[SIWIFI_RADAR_FCU] = NULL;
    }
}
bool siwifi_radar_set_domain(struct siwifi_radar *radar,
                           enum nl80211_dfs_regions region)
{
    if (radar->dpd[0] == NULL)
        return false;
    trace_radar_set_region(region);
    printk("radar set region %d\n",region);
    return (dfs_pattern_detector_set_domain(radar->dpd[SIWIFI_RADAR_RIU],
                                            region, SIWIFI_RADAR_RIU) &&
            dfs_pattern_detector_set_domain(radar->dpd[SIWIFI_RADAR_FCU],
                                            region, SIWIFI_RADAR_FCU));
}
void siwifi_radar_detection_enable(struct siwifi_radar *radar, u8 enable, u8 chain)
{
    if (chain < SIWIFI_RADAR_LAST ) {
        trace_radar_enable_detection(radar->dpd[chain]->region, enable, chain);
        spin_lock_bh(&radar->lock);
        radar->dpd[chain]->enabled = enable;
        spin_unlock_bh(&radar->lock);
    }
}
bool siwifi_radar_detection_is_enable(struct siwifi_radar *radar, u8 chain)
{
    return radar->dpd[chain]->enabled != SIWIFI_RADAR_DETECT_DISABLE;
}
void siwifi_radar_start_cac(struct siwifi_radar *radar, u32 cac_time_ms,
                          struct siwifi_vif *vif)
{
    WARN(radar->cac_vif != NULL, "CAC already in progress");
    radar->cac_vif = vif;
    schedule_delayed_work(&radar->cac_work, msecs_to_jiffies(cac_time_ms));
}
void siwifi_radar_cancel_cac(struct siwifi_radar *radar)
{
    struct siwifi_hw *siwifi_hw = container_of(radar, struct siwifi_hw, radar);
    if (radar->cac_vif == NULL) {
        return;
    }
    if (cancel_delayed_work_sync(&radar->cac_work)) {
        struct siwifi_chanctx *ctxt;
        ctxt = &siwifi_hw->chanctx_table[radar->cac_vif->ch_index];
  spin_unlock_bh(&siwifi_hw->cb_lock);
        siwifi_send_apm_stop_cac_req(siwifi_hw, radar->cac_vif);
  spin_lock_bh(&siwifi_hw->cb_lock);
        siwifi_chanctx_unlink(radar->cac_vif);
    }
    radar->cac_vif = NULL;
}
void siwifi_radar_detection_enable_on_cur_channel(struct siwifi_hw *siwifi_hw)
{
    struct siwifi_chanctx *ctxt;
    if (!siwifi_chanctx_valid(siwifi_hw, siwifi_hw->cur_chanctx))
        return;
    ctxt = &siwifi_hw->chanctx_table[siwifi_hw->cur_chanctx];
    if (ctxt->chan_def.chan->flags & IEEE80211_CHAN_RADAR) {
        siwifi_radar_detection_enable(&siwifi_hw->radar,
                                    SIWIFI_RADAR_DETECT_REPORT,
                                    SIWIFI_RADAR_RIU);
    } else {
        siwifi_radar_detection_enable(&siwifi_hw->radar,
                                    SIWIFI_RADAR_DETECT_DISABLE,
                                    SIWIFI_RADAR_RIU);
    }
}
static
int siwifi_radar_dump_pri_detector(char *buf, size_t len,
                                 struct pri_detector *pde)
{
    char freq_info[] = "Freq = %3.dMhz\n";
    char seq_info[] = " pri    | count | false \n";
    struct pri_sequence *seq;
    int res, write = 0;
    if (list_empty(&pde->sequences)) {
        return 0;
    }
    if (buf == NULL) {
        int nb_seq = 1;
        list_for_each_entry(seq, &pde->sequences, head) {
            nb_seq++;
        }
        return (sizeof(freq_info) + nb_seq * sizeof(seq_info));
    }
    res = scnprintf(buf, len, freq_info, pde->freq);
    write += res;
    len -= res;
    res = scnprintf(&buf[write], len, "%s", seq_info);
    write += res;
    len -= res;
    list_for_each_entry(seq, &pde->sequences, head) {
        res = scnprintf(&buf[write], len, " %6.d |   %2.d  |    %.2d \n",
                        seq->pri, seq->count, seq->count_falses);
        write += res;
        len -= res;
    }
    return write;
}
int siwifi_radar_dump_pattern_detector(char *buf, size_t len,
                                     struct siwifi_radar *radar, u8 chain)
{
    struct dfs_pattern_detector *dpd = radar->dpd[chain];
    char info[] = "Type = %3.d\n";
    struct pri_detector *pde;
    int i, res, write = 0;
    if (buf == NULL) {
        int size_needed = 0;
        for (i = 0; i < dpd->num_radar_types; i++) {
            list_for_each_entry(pde, &dpd->detectors[i], head) {
                size_needed += siwifi_radar_dump_pri_detector(NULL, 0, pde);
            }
            size_needed += sizeof(info);
        }
        return size_needed;
    }
    for (i = 0; i < dpd->num_radar_types; i++) {
        res = scnprintf(&buf[write], len, info, i);
        write += res;
        len -= res;
        list_for_each_entry(pde, &dpd->detectors[i], head) {
            res = siwifi_radar_dump_pri_detector(&buf[write], len, pde);
            write += res;
            len -= res;
        }
    }
    return write;
}
int siwifi_radar_dump_radar_detected(char *buf, size_t len,
                                   struct siwifi_radar *radar, u8 chain)
{
    struct siwifi_radar_detected *detect = &(radar->detected[chain]);
    char info[] = "2001/02/02 - 02:20 5126MHz\n";
    int idx, i, res, write = 0;
    int count = detect->count;
    if (count > NX_NB_RADAR_DETECTED)
        count = NX_NB_RADAR_DETECTED;
    if (buf == NULL) {
        return (count * sizeof(info)) + 1;
     }
    idx = (detect->index - detect->count) % NX_NB_RADAR_DETECTED;
    for (i = 0; i < count; i++) {
        struct tm tm;
        time64_to_tm(detect->time[idx], 0, &tm);
        res = scnprintf(&buf[write], len,
                        "%.4d/%.2d/%.2d - %.2d:%.2d %4.4dMHz\n",
                        (int)tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, detect->freq[idx]);
        write += res;
        len -= res;
        idx = (idx + 1 ) % NX_NB_RADAR_DETECTED;
    }
    return write;
}
