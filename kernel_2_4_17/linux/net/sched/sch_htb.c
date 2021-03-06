/* vim: ts=8 sw=4
 * net/sched/sch_htb.c	Hierarchical token bucket
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Martin Devera, <devik@cdi.cz>
 *
 * Credits (in time order):	
 *		Ondrej Kraus, <krauso@barr.cz> 
 *			found missing INIT_QDISC(htb)
 *		Vladimir Smelhaus, Aamer Akhter, Bert Hubert
 *			helped a lot to locate nasty class stall bug
 *		Andi Kleen, Jamal Hadi, Bert Hubert
 *			code review and helpful comments on shaping
 *		and many others. thanks.
 *
 * $Id: sch_htb.c,v 1.1 2004/05/11 17:38:59 cmo Exp $
 */

#include <linux/config.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/notifier.h>
#include <net/ip.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/pkt_sched.h>

/* HTB algorithm.
    Author: devik@cdi.cz
    =======================================
    HTB is like TBF with multiple classes. It is also similar to CBQ because
    it allows to assign priority to each class in hierarchy. 
    In fact it is another implementation os Floyd's formal sharing.

    Levels:
    Each class is assigned level. Leaf has ALWAYS level 0 and root 
    classes have level TC_HTB_MAXDEPTH-1. Interior nodes has level
    one less than their parent.
*/

#define HTB_HSIZE 16	/* classid hash size */
#define HTB_EWMAC 2	/* rate average over HTB_EWMAC*HTB_HSIZE sec */
#define HTB_DEBUG 1	/* compile debugging support (activated by tc tool) */
#define HTB_QLOCK(S) spin_lock_bh(&(S)->dev->queue_lock)
#define HTB_QUNLOCK(S) spin_unlock_bh(&(S)->dev->queue_lock)

/* ======== Begin of part to be deleted for 2.4 merged one ========= */
#if LINUX_VERSION_CODE < 0x20300
#define MODULE_LICENSE(X)

#define NET_XMIT_SUCCESS 1
#define NET_XMIT_DROP 0

static inline void __skb_queue_purge(struct sk_buff_head *list)
{
        struct sk_buff *skb;
	while ((skb=__skb_dequeue(list))!=NULL)
	    kfree_skb(skb);
}
#define del_timer_sync(t) del_timer(t)

#define netif_schedule qdisc_wakeup
#define netif_queue_stopped(D) (D->tbusy)
#define sch_tree_lock(S) start_bh_atomic()
#define sch_tree_unlock(S) end_bh_atomic()
#undef HTB_QLOCK
#undef HTB_QUNLOCK
#define HTB_QLOCK(S) 
#define HTB_QUNLOCK(S) 
#ifndef BUG_TRAP
#define BUG_TRAP(x) if (!(x)) { printk("Assertion (" #x ") failed at " __FILE__ "(%d):" __FUNCTION__ "\n", __LINE__); }
#endif
#endif
#if LINUX_VERSION_CODE < 0x20411 && !defined(CONFIG_RTNETLINK)
#error "CONFIG_RTNETLINK must be defined"
#endif
/* ======== End of part to be deleted for 2.4 merged one =========== */

/* debugging support; S is subsystem, these are defined:
  0 - netlink messages
  1 - enqueue
  2 - drop & requeue
  3 - dequeue main
  4 - dequeue one prio DRR part
  5 - dequeue class accounting
  6 - dequeue rcache (ready level computation)
 10 - rate estimator
 11 - classifier 
 12 - fast dequeue cache

 L is level; 0 = none, 1 = basic info, 2 = detailed, 3 = full
 q->debug uint32 contains 16 2-bit fields one for subsystem starting
 from LSB
 */
#if HTB_DEBUG
#define HTB_DBG(S,L,FMT,ARG...) if (((q->debug>>(2*S))&3) >= L) \
	printk(KERN_DEBUG FMT,##ARG)
#else
#define HTB_DBG(S,L,FMT,ARG...)
#endif


/* used internaly to pass status of single class */
enum htb_cmode {
    HTB_CANT_SEND,		/* class can't send and can't borrow */
    HTB_MAY_BORROW,		/* class can't send but may borrow */
    HTB_CAN_SEND		/* class can send */
};
#define HTB_F_INJ 0x10000	/* to mark dequeue level as injected one */

/* often used circular list of classes; I didn't use generic linux 
   double linked list to avoid casts and before I rely on some behaviour
   of insert and delete functions; item not bound to list is guaranted
   to have prev member NULL (we don't mangle next pointer as we often
   need it */
struct htb_litem {
    struct htb_class *prev, *next;
};
/* circular list insert and delete macros; these also maintain
   correct value of pointer to the list; insert adds 'new' class 
   before 'cl' class using prev/next member 'list' */
#define HTB_INSERTB(list,cl,new) \
do { if (!cl) new->list.prev = cl = new; \
    new->list.next = cl; new->list.prev = cl->list.prev; \
    cl->list.prev->list.next = cl->list.prev = new; } while(0)
    
/* remove 'cl' class from 'list' repairing 'ptr' if not null */
#define HTB_DELETE(list,cl,ptr) do { \
    if (cl->list.prev) { cl->list.prev->list.next = cl->list.next; \
    cl->list.next->list.prev = cl->list.prev; \
    if (ptr == cl) ptr = cl->list.next; \
    if (ptr == cl) ptr = NULL; cl->list.prev = NULL; } \
    else printk(KERN_ERR "htb: DELETE BUG [" #list "," #cl "," #ptr "]\n"); \
    } while (0)

/* interior & leaf nodes; props specific to leaves are marked L: */
struct htb_class
{
    /* general class parameters */
    u32 classid;
    struct tc_stats	stats;	/* generic stats */
    struct tc_htb_xstats xstats;/* our special stats */
    int refcnt;			/* usage count of this class */
    struct Qdisc *q;		/* L: elem. qdisc */

    /* rate measurement counters */
    unsigned long rate_bytes,sum_bytes;
    unsigned long rate_packets,sum_packets;

    /* DRR scheduler parameters */
    int quantum;	/* L: round quantum computed from rate */
    int deficit[TC_HTB_MAXDEPTH]; /* L: deficit for class at level */
    char prio;		/* L: priority of the class; 0 is the highest */
    char aprio;		/* L: prio at which we were last adding to active list
			   it is used to change priority at runtime */
    /* topology */
    char level;			/* our level (see above) */
    char injectd;		/* distance from injected parent */
    struct htb_class *parent;	/* parent class */
    struct htb_class *children;	/* pointer to children list */
    struct htb_litem hlist;	/* classid hash list */
    struct htb_litem active;	/* L: prio level active DRR list */
    struct htb_litem sibling;	/* sibling list */
    
    /* class attached filters */
    struct tcf_proto *filter_list;
    int filter_cnt;

    /* token bucket parameters */
    struct qdisc_rate_table *rate;	/* rate table of the class itself */
    struct qdisc_rate_table *ceil;	/* ceiling rate (limits borrows too) */
    long buffer,cbuffer;		/* token bucket depth/rate */
    long mbuffer;			/* max wait time */
    long tokens,ctokens;		/* current number of tokens */
    psched_time_t t_c;			/* checkpoint time */

    /* walk result cache for leaves */
    unsigned long rcache_sn;		/* SN of cache validity */
    unsigned rc_level;			/* victim's level */
};

/* TODO: maybe compute rate when size is too large .. or drop ? */
static __inline__ long L2T(struct htb_class *cl,struct qdisc_rate_table *rate,
	int size)
{ 
    int slot = size >> rate->rate.cell_log;
    if (slot > 255) {
	cl->xstats.giants++;
	slot = 255;
    }
    return rate->data[slot];
}

struct htb_sched
{
    struct htb_class *root;		/* root classes circular list */
    struct htb_class *hash[HTB_HSIZE];	/* hashed by classid */
    
    /* active classes table; this needs explanation. This table contains
       one set of pointers per priority, it is obvious. The set contains
       one pointer per class level in the same way as cl->deficit is
       independent for each level. This allows us to maintain correct
       DRR position independent of borrowing level.
       If we used single active/deficit items then DRR fairness'd suffer
       from frequent class level changes.
       Note that htb_[de]activate must be used to update this item
       because it needs to keep all pointers in set coherent. */
    struct htb_class *active[TC_HTB_NUMPRIO][TC_HTB_MAXDEPTH];

    int defcls;		/* class where unclassified flows go to */
    u32 debug;		/* subsystem debug levels */

    /* filters for qdisc itself */
    struct tcf_proto *filter_list;
    int filter_cnt;

    unsigned long sn;		/* result cache serial number */
    int rate2quantum;		/* quant = rate / rate2quantum */
    psched_time_t now;		/* cached dequeue time */
    long delay;			/* how long to deactivate for */
    struct timer_list timer;	/* send delay timer */
    struct timer_list rttim;	/* rate computer timer */
    int recmp_bucket;		/* which hash bucket to recompute next */

    /* cache of last dequeued class */
    struct htb_class *last_tx;
    int use_dcache;

    /* non shapped skbs; let them go directly thru */
    struct sk_buff_head direct_queue;
    int direct_qlen;  /* max qlen of above */

    /* statistics (see tc_htb_glob for explanation) */
    long deq_rate,deq_rate_c;
    long utilz,utilz_c;
    long trials,trials_c;
    long dcache_hits;
    long direct_pkts;
};

/* compute hash of size HTB_HSIZE for given handle */
static __inline__ int htb_hash(u32 h) 
{
#if HTB_HSIZE != 16
 #error "Declare new hash for your HTB_HSIZE"
#endif
    h ^= h>>8;	/* stolen from cbq_hash */
    h ^= h>>4;
    return h & 0xf;
}

/* find class in global hash table using given handle */
static __inline__ struct htb_class *htb_find(u32 handle, struct Qdisc *sch)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    int h = htb_hash(handle);
    struct htb_class *cl;
    if (TC_H_MAJ(handle) != sch->handle) return NULL;
    cl = q->hash[h];
    if (cl) do {
	if (cl->classid == handle) return cl;

    } while ((cl = cl->hlist.next) != q->hash[h]);
    return NULL;
}

/* classify packet into class TODO: use inner filters & marks here */
static struct htb_class *htb_clasify(struct sk_buff *skb, struct Qdisc *sch)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl;
    struct tcf_result res;
    struct tcf_proto *tcf;

    /* allow to select class by setting skb->priority to valid classid;
       note that nfmark can be used too by attaching filter fw with no
       rules in it */
    if (skb->priority == sch->handle)
	return NULL;  /* X:0 (direct flow) selected */
    if ((cl = htb_find(skb->priority,sch)) != NULL) 
	return cl;

    tcf = q->filter_list;
    while (tcf && !tc_classify(skb, tcf, &res)) {
	if (res.classid == sch->handle)
	    return NULL;  /* X:0 (direct flow) selected */
	if ((cl = htb_find(res.classid,sch)) == NULL)
	    break; /* filter selected invalid classid */
	if (!cl->level)
	    return cl; /* we hit leaf; return it */ 

	/* we have got inner class; apply inner filter chain */
	tcf = cl->filter_list;
    }
    /* classification failed; try to use default class */
    return htb_find(TC_H_MAKE(TC_H_MAJ(sch->handle),q->defcls),sch);
}

/* inserts cl into appropriate active lists (for all levels) */
static __inline__ void htb_activate(struct htb_sched *q,struct htb_class *cl)
{
    if (!cl->active.prev) {
	struct htb_class **ap = q->active[(int)(cl->aprio=cl->prio)];
	int i = !ap[0];
	HTB_INSERTB(active,ap[0],cl);
	if (i) /* set also all level pointers */
	    for (i = 1; i < TC_HTB_MAXDEPTH; i++) ap[i] = ap[0];
    }
}

/* remove cl from active lists; lev is level at which we dequeued
   so that we know that active[prio][lev] points to cl */
static __inline__ void 
htb_deactivate(struct htb_sched *q,struct htb_class *cl,int lev)
{
    int i;
    struct htb_class **ap = q->active[(int)cl->aprio];
    HTB_DELETE(active,cl,ap[lev]);
    if (ap[lev]) {
	/* repair other level pointers if they've pointed
	   to the deleted class */
	for (i = 0; i < TC_HTB_MAXDEPTH; i++)
	    if (ap[i] == cl) ap[i] = ap[lev];
    } else
	memset(ap,0,sizeof(*ap)*TC_HTB_MAXDEPTH);
}

static int htb_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = htb_clasify(skb,sch);

    if (!cl || !cl->q) {
	/* bad class; enqueue to helper queue */
	if (q->direct_queue.qlen < q->direct_qlen) {
	    __skb_queue_tail(&q->direct_queue, skb);
	    q->direct_pkts++;
	} else {
	    kfree_skb (skb);
	    sch->stats.drops++;
	    return NET_XMIT_DROP;
	}
    } else if (cl->q->enqueue(skb, cl->q) != NET_XMIT_SUCCESS) {
	sch->stats.drops++;
	cl->stats.drops++;
	return NET_XMIT_DROP;
    } else {
	cl->stats.packets++; cl->stats.bytes += skb->len;
	htb_activate (q,cl);
    }

    sch->q.qlen++;
    sch->stats.packets++; sch->stats.bytes += skb->len;
    HTB_DBG(1,1,"htb_enq_ok cl=%X skb=%p\n",cl?cl->classid:0,skb);
    return NET_XMIT_SUCCESS;
}

static int htb_requeue(struct sk_buff *skb, struct Qdisc *sch)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = htb_clasify(skb,sch);

    if (!cl || !cl->q) {
	/* bad class; enqueue to helper queue */
	if (q->direct_queue.qlen < q->direct_qlen) {
	    __skb_queue_tail(&q->direct_queue, skb);
	    q->direct_pkts++;
	} else {
	    kfree_skb (skb);
	    sch->stats.drops++;
	    return NET_XMIT_DROP;
	}
    } else if (cl->q->ops->requeue(skb, cl->q) != NET_XMIT_SUCCESS) {
	sch->stats.drops++;
	cl->stats.drops++;
	return NET_XMIT_DROP;
    } else 
    	htb_activate (q,cl);

    sch->q.qlen++;
    HTB_DBG(1,1,"htb_req_ok cl=%X skb=%p\n",cl?cl->classid:0,skb);
    return NET_XMIT_SUCCESS;
}

static void htb_timer(unsigned long arg)
{
    struct Qdisc *sch = (struct Qdisc*)arg;
    sch->flags &= ~TCQ_F_THROTTLED;
    wmb();
    netif_schedule(sch->dev);
}

#define RT_GEN(D,R) R+=D-(R/HTB_EWMAC);D=0
static void htb_rate_timer(unsigned long arg)
{
    struct Qdisc *sch = (struct Qdisc*)arg;
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl;

    /* lock queue so that we can muck with it */
    HTB_QLOCK(sch);
    HTB_DBG(10,1,"htb_rttmr j=%ld\n",jiffies);

    q->rttim.expires = jiffies + HZ;
    add_timer(&q->rttim);

    /* scan and recompute one bucket at time */
    if (++q->recmp_bucket >= HTB_HSIZE) q->recmp_bucket = 0;
    if ((cl = q->hash[q->recmp_bucket]) != NULL) do {
	HTB_DBG(10,2,"htb_rttmr_cl cl=%X sbyte=%lu spkt=%lu\n",cl->classid,cl->sum_bytes,cl->sum_packets);
	RT_GEN (cl->sum_bytes,cl->rate_bytes);
	RT_GEN (cl->sum_packets,cl->rate_packets);
    } while ((cl = cl->hlist.next) != q->hash[q->recmp_bucket]);
    
    /* global stats */
    RT_GEN (q->trials_c,q->trials);
    RT_GEN (q->utilz_c,q->utilz);
    RT_GEN (q->deq_rate_c,q->deq_rate);

    HTB_QUNLOCK(sch);
}

/* test whether class can send or borrow packet */
static enum htb_cmode
htb_class_mode(struct htb_sched *q, struct htb_class *cl)
{
    long toks,diff;
    diff = PSCHED_TDIFF_SAFE(q->now, cl->t_c, (u32)cl->mbuffer, 0);
    HTB_DBG(6,3,"htb_cm diff=%ld\n",diff);

    /* check whether we are over ceil */
    if ((toks = (cl->ctokens + diff)) < 0) { 
	if (q->delay > -toks || !q->delay) q->delay = -toks;
	return HTB_CANT_SEND;
    }

    /* our regular rate */
    if ((toks = (cl->tokens + diff)) >= 0) 
	return HTB_CAN_SEND;

    /* record time when we can transmit */
    if (q->delay > -toks || !q->delay) q->delay = -toks;

    return HTB_MAY_BORROW;
}

/* computes (possibly ancestor) class ready to send; cl is leaf; 
   cl's rc_level is then filled with level we are borrowing at; 
   it is set to TC_HTB_MAXDEPTH if we can't borrow at all and can be
   ORed with HTB_F_INJ if bw was injected. */
static void htb_ready_level(struct htb_sched *q,struct htb_class *cl)
{
    struct htb_class *stack[TC_HTB_MAXDEPTH],**sp = stack;
    int level = TC_HTB_MAXDEPTH, injdist = cl->injectd;
    enum htb_cmode mode;
    HTB_DBG(6,1,"htb_rl cl=%X tok=%ld ctok=%ld buf=%ld cbuf=%ld\n",cl->classid,cl->tokens,cl->ctokens,cl->buffer,cl->cbuffer);

    /* traverse tree upward looking for ready class */
    for (;;) {
	*sp++ = cl; /* push at stack */

	/* test mode */
	mode = htb_class_mode(q,cl);
	HTB_DBG(6,2,"htb_clmod cl=%X m=%d tok=%ld ctok=%ld buf=%ld cbuf=%ld\n",cl->classid,mode,cl->tokens,cl->ctokens,cl->buffer,cl->cbuffer);
	if (mode != HTB_MAY_BORROW) {
	    if (mode == HTB_CAN_SEND) level = cl->level;
	    break;
	}
	/* update injdist from current node */
	if (injdist > cl->injectd) injdist = cl->injectd;

	/* if this is leaf's injector then resolve borrow positively */
	if (!injdist--) {
	    /* don't cache this result in interior nodes */
	    stack[0]->rc_level = cl->level|HTB_F_INJ;
	    stack[0]->rcache_sn = q->sn;
	    return;
	}
	if ((cl = cl->parent) == NULL) break;
	if (q->sn == cl->rcache_sn) {
	    /* the node has already computed result; use it */
	    level = cl->rc_level; break;
	}
    }
    while (--sp >= stack) { /* update mode cache */
	(*sp)->rcache_sn = q->sn;
	(*sp)->rc_level = level;
    }	
}

/* pull packet from class and charge to ancestors */
static struct sk_buff *
htb_dequeue_class(struct Qdisc *sch, struct htb_class *cl)
{	
    struct htb_sched *q = (struct htb_sched *)sch->data;
    long toks,diff;
    int injecting = cl->rc_level & HTB_F_INJ, injdist = cl->injectd;
    int level = cl->rc_level & 0xff;
    struct sk_buff *skb = cl->q->dequeue(cl->q);
    HTB_DBG(5,1,"htb_deq_cl cl=%X skb=%p lev=%d inj=%d\n",cl->classid,skb,level,injecting);
    if (!skb) return NULL;

    /* we have got skb, account it to victim and its parents
       and also to all ceil estimators under victim */
    while (cl) {
	diff = PSCHED_TDIFF_SAFE(q->now, cl->t_c, (u32)cl->mbuffer, 0);

#define HTB_ACCNT(T,B,R) toks = diff + cl->T; \
	if (toks > cl->B) toks = cl->B; \
	    toks -= L2T(cl, cl->R, skb->len); \
	if (toks <= -cl->mbuffer) toks = 1-cl->mbuffer; \
	    cl->T = toks

	HTB_ACCNT (ctokens,cbuffer,ceil);
	if (cl->level >= level) {
	    if (cl->level == level) cl->xstats.lends++;
	    HTB_ACCNT (tokens,buffer,rate);
	} else {
	    cl->xstats.borrows++;
	    cl->tokens += diff; /* we moved t_c; update tokens */
	}
	cl->t_c = q->now;
	HTB_DBG(5,2,"htb_deq_clp cl=%X clev=%d diff=%ld\n",cl->classid,cl->level,diff);

	/* update rate counters */
	cl->sum_bytes += skb->len; cl->sum_packets++;

	/* update byte stats except for leaves which are already updated */
	if (cl->level) {
	    cl->stats.bytes += skb->len;
	    cl->stats.packets++;
	}
	/* finish if we hit stop-class and we are injecting */
	if (injecting) {
	    if (injdist > cl->injectd) injdist = cl->injectd;
	    if (!injdist--) {
		cl->xstats.injects++; break;
	    }
	}
	cl = cl->parent;
    }
    return skb;
}

/* dequeues packet at given priority borrowing from given level;
   if unsuccessfull then it returns level at which someone can
   dequeue. If it sets level to TC_HTB_MAXDEPTH then no one can. */
static struct sk_buff *htb_dequeue_prio(struct Qdisc *sch,int prio,int *level)
{
    struct sk_buff *skb = NULL;
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class **ap = q->active[prio], *cl = ap[*level];
    int done,top = TC_HTB_MAXDEPTH,rclev;

    HTB_DBG(4,1,"htb_deq_pr pr=%d lev=%d cl=%X\n",prio,*level,cl->classid);
    /* this is DRR algorithm */
    do {
	done = 1;
	do {
	    /* catch empty classes here; note that we don't remove them
	       immediately after dequeue but rather delay remove to next
	       DRR round because if packet arrive for just emptied class
	       then we don't need to remove and again add it */
	    if (!cl->q->q.qlen) {
		ap[*level] = cl; /* needed for HTB_DELETE in deactivate */
		htb_deactivate (q,cl,*level);

		HTB_DBG(4,2,"htb_deq_deact cl=%X ncl=%X\n",cl->classid,ap[*level]?ap[*level]->classid:0);
		if (ap[*level]) continue;
		*level = TC_HTB_MAXDEPTH; 
		return NULL; /* NO class remains active */
	    }
	    /* test whether class can send at all borrowing from level */
	    if (cl->rcache_sn != q->sn) htb_ready_level(q,cl);
	    rclev = cl->rc_level & 0xff; /* filter injecting flag out */
	    
	    HTB_DBG(4,2,"htb_deq_rd cl=%X rc_lev=0x%x dfct=%d qnt=%d\n",
			cl->classid,cl->rc_level,cl->deficit[*level],cl->quantum);

	    if (rclev == TC_HTB_MAXDEPTH) {
		/* TODO: overlimit increment here is not proven correct */
		if (cl->deficit[*level] > 0) cl->stats.overlimits++;
		continue; /* can't send or borrow */
	    }
	    /* if we can't send at this level, remember where we can */
	    if (rclev > *level) {
		if (rclev < top) /* keep lowest level */
		    top = rclev;
		
		HTB_DBG(4,2,"htb_deq_badlev top=%d\n",top);
		continue;
	    }
	    if (cl->deficit[*level] <= 0) {
		/* haven't allotment, increase and try again */
		done = 0; cl->deficit[*level] += cl->quantum;
		continue;
	    }
	    if ((skb = htb_dequeue_class(sch,cl)) == NULL) {
		/* nonempty class can't dequeue so that mark it as such;
		   note that rcache_sn is already set and thus this remarking
		   will be valid only for rest of this dequeue; this is
		   possible if child class is non work conserving */
		cl->rc_level = TC_HTB_MAXDEPTH;

		HTB_DBG(4,2,"htb_deq_noskb cl=%X len=%d\n",cl->classid,cl->q->q.qlen);
		continue;
	    }
	    sch->q.qlen--;
	    /* prepare next class if we can't stay valid */
	    if ((cl->deficit[*level] -= skb->len) <= 0) cl = cl->active.next;
	    else if (q->use_dcache) 
		q->last_tx = cl; /* cache cl if it still can transmit */
	    ap[*level] = cl;
	    
	    HTB_DBG(4,1,"htb_deq_haspkt ncl=%X sqlen=%d\n",cl->classid,sch->q.qlen);
	    return skb;

	} while ((cl = cl->active.next) != ap[*level]);

    } while (!done);
    *level = top; 
    HTB_DBG(4,1,"htb_deq_quit top=%d\n",top);
    return NULL;
}

static struct sk_buff *htb_dequeue(struct Qdisc *sch)
{
    struct sk_buff *skb = NULL;
    struct htb_sched *q = (struct htb_sched *)sch->data;
    int prio,oklev,okprio = 0 /* avoid unused warning */,lev,i;
    struct htb_class *cl;
    psched_time_t endt;
    
    HTB_DBG(3,1,"htb_deq dircnt=%d ltx=%X\n",skb_queue_len(&q->direct_queue),
	    q->last_tx?q->last_tx->classid:0);

    /* try to dequeue direct packets as high prio (!) to minimize cpu work */
    if ((skb = __skb_dequeue(&q->direct_queue)) != NULL) {
	sch->flags &= ~TCQ_F_THROTTLED;
	sch->q.qlen--;
	return skb;
    }

    PSCHED_GET_TIME(q->now); /* htb_dequeue_class needs it too */
    q->delay = 0; q->sn++;

    /* well here I bite CBQ's speed :-) if last dequeued class is
       still active and is not deficit then we can dequeue it again */
    if ((cl = q->last_tx) != NULL && cl->q->q.qlen > 0 && 
	    cl->deficit[cl->rc_level & 0xff] > 0 && 
	    (skb = htb_dequeue_class(sch,cl)) != NULL) {
	sch->q.qlen--;
	cl->deficit[cl->rc_level & 0xff] -= skb->len;
	sch->flags &= ~TCQ_F_THROTTLED;
	q->dcache_hits++;
	HTB_DBG(3,1,"htb_deq_hit skb=%p\n",skb);
	return skb;
    }
    q->last_tx = NULL; /* can't use cache ? invalidate */
    
    for (i = 0; i < TC_HTB_MAXDEPTH; i++) {
	/* first try: dequeue leaves (level 0) */
	oklev = TC_HTB_MAXDEPTH;
	q->trials_c++;
	for (prio = 0; prio < TC_HTB_NUMPRIO; prio++) {
	    if (!q->active[prio][0]) continue;
	    lev = 0; skb = htb_dequeue_prio(sch, prio, &lev);
	    HTB_DBG(3,2,"htb_deq_1 i=%d p=%d skb=%p blev=%d\n",i,prio,skb,lev);
	    if (skb) {
		sch->flags &= ~TCQ_F_THROTTLED;
		goto fin;
	    }
	    if (lev < oklev) {
		oklev = lev; okprio = prio;
	    }
	}
	if (oklev >= TC_HTB_MAXDEPTH) break; 
	/* second try: use ok level we learned in first try; 
	   it really should succeed */
	q->trials_c++;
	skb = htb_dequeue_prio(sch, okprio, &oklev);
	HTB_DBG(3,2,"htb_deq_2 p=%d lev=%d skb=%p\n",okprio,oklev,skb);
	if (skb) {
	    sch->flags &= ~TCQ_F_THROTTLED;
	    goto fin;
	}
	/* probably qdisc at oklev can't transmit - it is not good
	   idea to have TBF as HTB's child ! retry with that node
	   disabled */
    }
    if (i >= TC_HTB_MAXDEPTH)
	printk(KERN_ERR "htb: too many dequeue trials\n");

    /* no-one gave us packet, setup timer if someone wants it */
    if (sch->q.qlen && !netif_queue_stopped(sch->dev) && q->delay) {
	long delay = PSCHED_US2JIFFIE(q->delay);
	if (delay == 0) delay = 1;
	if (delay > 5*HZ) {
	    if (net_ratelimit())
		printk(KERN_INFO "HTB delay %ld > 5sec\n", delay);
	    delay = 5*HZ;
	}
	del_timer(&q->timer);
	q->timer.expires = jiffies + delay;
	add_timer(&q->timer);
	sch->flags |= TCQ_F_THROTTLED;
	sch->stats.overlimits++;
	HTB_DBG(3,1,"htb_deq t_delay=%ld\n",delay);
    }
fin:
    do {
	static unsigned util = 0; unsigned d;
	PSCHED_GET_TIME(endt); q->deq_rate_c++;
	d = PSCHED_TDIFF(endt,q->now);
	q->utilz_c += d; util += d;
#if 0
	/* special debug hack */
	if (skb) { 
	    memcpy (skb->data+28,_dbg,sizeof(_dbg));
	    memset (_dbg,0,sizeof(_dbg));
	}
#endif
    } while (0);
    HTB_DBG(3,1,"htb_deq_end %s j=%lu skb=%p (28/11)\n",sch->dev->name,jiffies,skb);
    return skb;
}

/* try to drop from each class (by prio) until one succeed */
static int htb_drop(struct Qdisc* sch)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    int prio;

    for (prio = TC_HTB_NUMPRIO - 1; prio >= 0; prio--) {
	struct htb_class *cl = q->active[prio][0];
	if (cl) do {
	    if (cl->q->ops->drop && cl->q->ops->drop(cl->q)) {
		sch->q.qlen--;
		return 1;
	    }
	} while ((cl = cl->active.next) != q->active[prio][0]);
    }
    return 0;
}

/* reset all classes */
/* always caled under BH & queue lock */
static void htb_reset(struct Qdisc* sch)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    int i;
    HTB_DBG(0,1,"htb_reset sch=%X, handle=%X\n",(int)sch,sch->handle);

    for (i = 0; i < HTB_HSIZE; i++) {
	struct htb_class *cl = q->hash[i];
	if (cl) do {
	    if (cl->q) qdisc_reset(cl->q);

	} while ((cl = cl->hlist.next) != q->hash[i]);
    }
    sch->flags &= ~TCQ_F_THROTTLED;
    del_timer(&q->timer);
    __skb_queue_purge(&q->direct_queue);
    sch->q.qlen = 0; q->last_tx = NULL;
}

static int htb_init(struct Qdisc *sch, struct rtattr *opt)
{
    struct htb_sched *q = (struct htb_sched*)sch->data;
    struct rtattr *tb[TCA_HTB_INIT];
    struct tc_htb_glob *gopt;

    if (!opt || 
	    rtattr_parse(tb, TCA_HTB_INIT, RTA_DATA(opt), RTA_PAYLOAD(opt)) ||
	    tb[TCA_HTB_INIT-1] == NULL ||
	    RTA_PAYLOAD(tb[TCA_HTB_INIT-1]) < sizeof(*gopt))
	return -EINVAL;

    gopt = RTA_DATA(tb[TCA_HTB_INIT-1]);
    memset(q,0,sizeof(*q));
    q->debug = gopt->debug;
    HTB_DBG(0,1,"htb_init sch=%p handle=%X r2q=%d\n",sch,sch->handle,gopt->rate2quantum);
    init_timer(&q->timer);
    init_timer(&q->rttim);
    skb_queue_head_init(&q->direct_queue);
    q->direct_qlen = sch->dev->tx_queue_len;
    q->timer.function = htb_timer;
    q->timer.data = (unsigned long)sch;
    q->rttim.function = htb_rate_timer;
    q->rttim.data = (unsigned long)sch;
    q->rttim.expires = jiffies + HZ;
    add_timer(&q->rttim);
    if ((q->rate2quantum = gopt->rate2quantum) < 1)
	q->rate2quantum = 1;
    q->defcls = gopt->defcls;
    q->use_dcache = gopt->use_dcache;

    MOD_INC_USE_COUNT;
    return 0;
}

static int htb_dump(struct Qdisc *sch, struct sk_buff *skb)
{
    struct htb_sched *q = (struct htb_sched*)sch->data;
    unsigned char	 *b = skb->tail;
    struct rtattr *rta;
    struct tc_htb_glob gopt;
    HTB_DBG(0,1,"htb_dump sch=%p, handle=%X\n",sch,sch->handle);
    /* stats */
    HTB_QLOCK(sch);
    gopt.deq_rate = q->deq_rate/HTB_EWMAC;
    gopt.utilz = q->utilz/HTB_EWMAC;
    gopt.trials = q->trials/HTB_EWMAC;
    gopt.dcache_hits = q->dcache_hits;
    gopt.direct_pkts = q->direct_pkts;

    gopt.use_dcache = q->use_dcache;
    gopt.rate2quantum = q->rate2quantum;
    gopt.defcls = q->defcls;
    gopt.debug = q->debug;
    rta = (struct rtattr*)b;
    RTA_PUT(skb, TCA_OPTIONS, 0, NULL);
    RTA_PUT(skb, TCA_HTB_INIT, sizeof(gopt), &gopt);
    rta->rta_len = skb->tail - b;
    sch->stats.qlen = sch->q.qlen;
    RTA_PUT(skb, TCA_STATS, sizeof(sch->stats), &sch->stats);
    HTB_QUNLOCK(sch);
    return skb->len;
rtattr_failure:
    HTB_QUNLOCK(sch);
    skb_trim(skb, skb->tail - skb->data);
    return -1;
}

static int
htb_dump_class(struct Qdisc *sch, unsigned long arg,
	struct sk_buff *skb, struct tcmsg *tcm)
{
    struct htb_sched *q = (struct htb_sched*)sch->data;
    struct htb_class *cl = (struct htb_class*)arg;
    unsigned char	 *b = skb->tail;
    struct rtattr *rta;
    struct tc_htb_opt opt;

    HTB_DBG(0,1,"htb_dump_class handle=%X clid=%X\n",sch->handle,cl->classid);

    HTB_QLOCK(sch);
    tcm->tcm_parent = cl->parent ? cl->parent->classid : TC_H_ROOT;
    tcm->tcm_handle = cl->classid;
    if (cl->q) {
	tcm->tcm_info = cl->q->handle;
	cl->stats.qlen = cl->q->q.qlen;
    }

    rta = (struct rtattr*)b;
    RTA_PUT(skb, TCA_OPTIONS, 0, NULL);

    memset (&opt,0,sizeof(opt));

    opt.rate = cl->rate->rate; opt.buffer = cl->buffer;
    opt.ceil = cl->ceil->rate; opt.cbuffer = cl->cbuffer;
    opt.quantum = cl->quantum; opt.prio = cl->prio;
    opt.level = cl->level; opt.injectd = cl->injectd;
    RTA_PUT(skb, TCA_HTB_PARMS, sizeof(opt), &opt);
    rta->rta_len = skb->tail - b;

    cl->stats.bps = cl->rate_bytes/(HTB_EWMAC*HTB_HSIZE);
    cl->stats.pps = cl->rate_packets/(HTB_EWMAC*HTB_HSIZE);

    cl->xstats.tokens = cl->tokens;
    cl->xstats.ctokens = cl->ctokens;
    RTA_PUT(skb, TCA_STATS, sizeof(cl->stats), &cl->stats);
    RTA_PUT(skb, TCA_XSTATS, sizeof(cl->xstats), &cl->xstats);
    HTB_QUNLOCK(sch);
    return skb->len;
rtattr_failure:
    HTB_QUNLOCK(sch);
    skb_trim(skb, b - skb->data);
    return -1;
}

static int htb_graft(struct Qdisc *sch, unsigned long arg, struct Qdisc *new,
	struct Qdisc **old)
{
    struct htb_class *cl = (struct htb_class*)arg;

    if (cl && !cl->level) {
	if (new == NULL) {
	    if ((new = qdisc_create_dflt(sch->dev, &pfifo_qdisc_ops)) == NULL)
		return -ENOBUFS;
	} 
	if ((*old = xchg(&cl->q, new)) != NULL) /* xchg is atomical :-) */
	    qdisc_reset(*old);
	return 0;
    }
    return -ENOENT;
}

static struct Qdisc *
htb_leaf(struct Qdisc *sch, unsigned long arg)
{
    struct htb_class *cl = (struct htb_class*)arg;
    return cl ? cl->q : NULL;
}

static unsigned long htb_get(struct Qdisc *sch, u32 classid)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = htb_find(classid,sch);
    HTB_DBG(0,1,"htb_get clid=%X q=%p cl=%p ref=%d\n",classid,q,cl,cl?cl->refcnt:0);
    if (cl) cl->refcnt++;
    return (unsigned long)cl;
}

static void htb_destroy_filters(struct tcf_proto **fl)
{
    struct tcf_proto *tp;

    while ((tp = *fl) != NULL) {
	*fl = tp->next;
	tp->ops->destroy(tp);
    }
}

static void htb_destroy_class(struct Qdisc* sch,struct htb_class *cl)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    HTB_DBG(0,1,"htb_destrycls clid=%X q=%p ref=%d\n", cl?cl->classid:0,cl->q,cl?cl->refcnt:0);
    if (cl->q) qdisc_destroy(cl->q);
    qdisc_put_rtab(cl->rate);
    qdisc_put_rtab(cl->ceil);
#ifdef CONFIG_NET_ESTIMATOR
    qdisc_kill_estimator(&cl->stats);
#endif
    htb_destroy_filters (&cl->filter_list);
    /* remove children */
    while (cl->children) htb_destroy_class (sch,cl->children);

    /* remove class from all lists it is on */
    q->last_tx = NULL;
    if (cl->hlist.prev)
	HTB_DELETE(hlist,cl,q->hash[htb_hash(cl->classid)]);
    if (cl->active.prev) 
	htb_deactivate (q,cl,0);
    if (cl->parent) 
	HTB_DELETE(sibling,cl,cl->parent->children);
    else
	HTB_DELETE(sibling,cl,q->root);

    kfree(cl);
}

/* always caled under BH & queue lock */
static void htb_destroy(struct Qdisc* sch)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    HTB_DBG(0,1,"htb_destroy q=%p\n",q);

    del_timer_sync (&q->timer);
    del_timer_sync (&q->rttim);
    while (q->root) htb_destroy_class(sch,q->root);
    htb_destroy_filters(&q->filter_list);
    __skb_queue_purge(&q->direct_queue);
    MOD_DEC_USE_COUNT;
}

static void htb_put(struct Qdisc *sch, unsigned long arg)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = (struct htb_class*)arg;
    HTB_DBG(0,1,"htb_put q=%p cl=%X ref=%d\n",q,cl?cl->classid:0,cl?cl->refcnt:0);

    if (--cl->refcnt == 0)
	htb_destroy_class(sch,cl);
}

static int
htb_change_class(struct Qdisc *sch, u32 classid, u32 parentid, struct rtattr **tca,
	unsigned long *arg)
{
    int err = -EINVAL,h;
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = (struct htb_class*)*arg,*parent;
    struct rtattr *opt = tca[TCA_OPTIONS-1];
    struct qdisc_rate_table *rtab = NULL, *ctab = NULL;
    struct rtattr *tb[TCA_HTB_RTAB];
    struct tc_htb_opt *hopt;

    if (parentid == TC_H_ROOT) parent = NULL; 
    else parent = htb_find (parentid,sch);

    /* extract all subattrs from opt attr */
    if (!opt || 
	    rtattr_parse(tb, TCA_HTB_RTAB, RTA_DATA(opt), RTA_PAYLOAD(opt)) ||
	    tb[TCA_HTB_PARMS-1] == NULL ||
	    RTA_PAYLOAD(tb[TCA_HTB_PARMS-1]) < sizeof(*hopt))
	goto failure;

    hopt = RTA_DATA(tb[TCA_HTB_PARMS-1]);
    HTB_DBG(0,1,"htb_chg cl=%p, clid=%X, opt/prio=%d, rate=%u, buff=%d, quant=%d\n", cl,cl?cl->classid:0,(int)hopt->prio,hopt->rate.rate,hopt->buffer,hopt->quantum);
    rtab = qdisc_get_rtab(&hopt->rate, tb[TCA_HTB_RTAB-1]);
    ctab = qdisc_get_rtab(&hopt->ceil, tb[TCA_HTB_CTAB-1]);
    if (!rtab || !ctab) goto failure;

    if (!cl) { /* new class */
	/* check maximal depth */
	if (parent && parent->parent && parent->parent->level < 2) {
	    printk(KERN_ERR "htb: tree is too deep\n");
	    goto failure;
	}
	err = -ENOBUFS;
	cl = kmalloc(sizeof(*cl), GFP_KERNEL);
	if (cl == NULL) goto failure;
	memset(cl, 0, sizeof(*cl));
	cl->refcnt = 1; cl->level = 0; /* assume leaf */

	if (parent && !parent->level) {
	    /* turn parent into inner node */
	    qdisc_destroy (parent->q); parent->q = &noop_qdisc;
	    parent->level = (parent->parent ? 
		    parent->parent->level : TC_HTB_MAXDEPTH) - 1;
	}
	/* leaf (we) needs elementary qdisc */
	if (!(cl->q = qdisc_create_dflt(sch->dev, &pfifo_qdisc_ops)))
	    cl->q = &noop_qdisc;

	cl->classid = classid; cl->parent = parent;
	cl->tokens = hopt->buffer;
	cl->ctokens = hopt->cbuffer;
	cl->mbuffer = 60000000; /* 1min */
	PSCHED_GET_TIME(cl->t_c);

	/* attach to the hash list and parent's family */
	sch_tree_lock(sch);
	h = htb_hash(classid);
	if (!cl->hlist.prev)
	    HTB_INSERTB(hlist,q->hash[h],cl);
	if (parent) 
	    HTB_INSERTB(sibling,parent->children,cl);
	else HTB_INSERTB(sibling,q->root,cl);

    } else sch_tree_lock(sch);

    q->last_tx = NULL;
    cl->quantum = rtab->rate.rate / q->rate2quantum;
    cl->injectd = hopt->injectd;
    if (cl->quantum < 100) cl->quantum = 100;
    if (cl->quantum > 60000) cl->quantum = 60000;
    if ((cl->prio = hopt->prio) >= TC_HTB_NUMPRIO)
	cl->prio = TC_HTB_NUMPRIO - 1;

    cl->buffer = hopt->buffer;
    cl->cbuffer = hopt->cbuffer;
    if (cl->rate) qdisc_put_rtab(cl->rate); cl->rate = rtab;
    if (cl->ceil) qdisc_put_rtab(cl->ceil); cl->ceil = ctab;
    sch_tree_unlock(sch);

    *arg = (unsigned long)cl;
    return 0;

failure:
    if (rtab) qdisc_put_rtab(rtab);
    if (ctab) qdisc_put_rtab(ctab);
    return err;
}

static int htb_delete(struct Qdisc *sch, unsigned long arg)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = (struct htb_class*)arg;
    HTB_DBG(0,1,"htb_delete q=%p cl=%X ref=%d\n",q,cl?cl->classid:0,cl?cl->refcnt:0);

    if (cl->children || cl->filter_cnt) return -EBUSY;
    sch_tree_lock(sch);
    /* delete from hash and active; remainder in destroy_class */
    if (cl->hlist.prev)
	HTB_DELETE(hlist,cl,q->hash[htb_hash(cl->classid)]);
    if (cl->active.prev) 
	htb_deactivate (q,cl,0);
    q->last_tx = NULL;

    if (--cl->refcnt == 0)
	htb_destroy_class(sch,cl);

    sch_tree_unlock(sch);
    return 0;
}

static struct tcf_proto **htb_find_tcf(struct Qdisc *sch, unsigned long arg)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = (struct htb_class *)arg;
    struct tcf_proto **fl = cl ? &cl->filter_list : &q->filter_list;
    HTB_DBG(0,2,"htb_tcf q=%p clid=%X fref=%d fl=%p\n",q,cl?cl->classid:0,cl?cl->filter_cnt:q->filter_cnt,*fl);
    return fl;
}

static unsigned long htb_bind_filter(struct Qdisc *sch, unsigned long parent,
	u32 classid)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = htb_find (classid,sch);
    HTB_DBG(0,2,"htb_bind q=%p clid=%X cl=%p fref=%d\n",q,classid,cl,cl?cl->filter_cnt:q->filter_cnt);
    /*if (cl && !cl->level) return 0;
      The line above used to be there to prevent attachind filters to leaves. But
      at least tc_index filter uses this just to get class for other reasons so
      that we have to allow for it.
      */
    if (cl) cl->filter_cnt++; else q->filter_cnt++;
    return (unsigned long)cl;
}

static void htb_unbind_filter(struct Qdisc *sch, unsigned long arg)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    struct htb_class *cl = (struct htb_class *)arg;
    HTB_DBG(0,2,"htb_unbind q=%p cl=%p fref=%d\n",q,cl,cl?cl->filter_cnt:q->filter_cnt);
    if (cl) cl->filter_cnt--; else q->filter_cnt--;
}

static void htb_walk(struct Qdisc *sch, struct qdisc_walker *arg)
{
    struct htb_sched *q = (struct htb_sched *)sch->data;
    int i;

    if (arg->stop)
	return;

    for (i = 0; i < HTB_HSIZE; i++) {
	struct htb_class *cl = q->hash[i];
	if (cl) do {
	    if (arg->count < arg->skip) {
		arg->count++;
		continue;
	    }
	    if (arg->fn(sch, (unsigned long)cl, arg) < 0) {
		arg->stop = 1;
		return;
	    }
	    arg->count++;

	} while ((cl = cl->hlist.next) != q->hash[i]);
    }
}

static struct Qdisc_class_ops htb_class_ops =
{
    htb_graft,
    htb_leaf,
    htb_get,
    htb_put,
    htb_change_class,
    htb_delete,
    htb_walk,

    htb_find_tcf,
    htb_bind_filter,
    htb_unbind_filter,

    htb_dump_class,
};

struct Qdisc_ops htb_qdisc_ops =
{
    NULL,
    &htb_class_ops,
    "htb",
    sizeof(struct htb_sched),

    htb_enqueue,
    htb_dequeue,
    htb_requeue,
    htb_drop,

    htb_init,
    htb_reset,
    htb_destroy,
    NULL /* htb_change */,

    htb_dump,
};

#ifdef MODULE
int init_module(void)
{
    return register_qdisc(&htb_qdisc_ops);
}

void cleanup_module(void) 
{
    unregister_qdisc(&htb_qdisc_ops);
}
MODULE_LICENSE("GPL");
#endif
