//========================================================================
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================

#include <cpsw3g_miniport.h>
#include <Am33xCpsw3gRegs.h>

#define u32 UINT32 
#define CPSW_ADDR_LEN ETH_LENGTH_OF_ADDRESS

/* ALE Table Definitions and Bitfields */
#define CPSW_ALE_ENTRY_BITS			68
#define CPSW_ALE_ENTRY_WORDS			\
	((CPSW_ALE_ENTRY_BITS + 31) / 32)
#define CPSW_ALE_ENTRY_TYPE_SHIFT		60
#define CPSW_ALE_ENTRY_TYPE_BITS		2
#define CPSW_ALE_VLAN_ID_SHIFT			48
#define CPSW_ALE_VLAN_ID_BITS			12
#define CPSW_ALE_ADDR_SHIFT			0
#define CPSW_ALE_ADDR_BITS			48
#define CPSW_ALE_MCAST_STATE_SHIFT		62
#define CPSW_ALE_MCAST_STATE_BITS		2
#define CPSW_ALE_PORT_MASK_SHIFT		64
#define CPSW_ALE_PORT_MASK_BITS			3
#define CPSW_ALE_SUPER_SHIFT			67
#define CPSW_ALE_SUPER_BITS			1
#define CPSW_ALE_MCAST_SHIFT			40
#define CPSW_ALE_MCAST_BITS			1
#define CPSW_ALE_UCAST_TYPE_SHIFT		66
#define CPSW_ALE_UCAST_TYPE_BITS		2
#define CPSW_ALE_PORT_NUM_SHIFT			64
#define CPSW_ALE_PORT_NUM_BITS			2
#define CPSW_ALE_BLOCKED_SHIFT			63
#define CPSW_ALE_BLOCKED_BITS			1
#define CPSW_ALE_SECURE_SHIFT			62
#define CPSW_ALE_SECURE_BITS			1
#define CPSW_ALE_UCAST_TYPE_SHIFT		66
#define CPSW_ALE_UCAST_TYPE_BITS		2
#define CPSW_ALE_VLAN_MEM_LIST_SHIFT 0
#define CPSW_ALE_VLAN_MEM_LIST_BITS   3
#define CPSW_ALE_VLAN_UNREG_FLOOD_SHIFT 8
#define CPSW_ALE_VLAN_UNREG_FLOOD_BITS   3
#define CPSW_ALE_VLAN_REG_FLOOD_SHIFT 16
#define CPSW_ALE_VLAN_REG_FLOOD_BITS   3
#define CPSW_ALE_VLAN_FORCE_UNTAG_EGRESS_SHIFT 24
#define CPSW_ALE_VLAN_FORCE_UNTAG_EGRESS_BITS   3

/* Miscellaneous register related defines */
#define CPSW_ALE_TABLE_SIZE			1024
#define CPSW_ALE_TABLE_WRITE			(0x1 << 31)

#define CPSW_MCAST(mac)		((mac)[0] & 0x01)
#define CPSW_ALE_BITMASK(bits)	((1 << (bits)) - 1)
#define CPSW_BCAST(mac)              ((mac[0]==0xff) && (mac[1]==0xff) && (mac[2]==0xff) && \
                                                          (mac[3]==0xff) && (mac[4]==0xff) && (mac[5]==0xff))

#define CPSW_ALE_GET_FIELD(val, field)		\
	cpsw_ale_get_field(val,			\
		CPSW_ALE_ ## field ## _SHIFT,	\
		CPSW_ALE_ ## field ## _BITS)

#define CPSW_ALE_SET_FIELD(val, field, fval)	\
	cpsw_ale_set_field(val,			\
		CPSW_ALE_ ## field ## _SHIFT,	\
		CPSW_ALE_ ## field ## _BITS,	\
		fval)

enum cpsw_ale_entry_type {
	CPSW_ALE_FREE = 0,
	CPSW_ALE_ADDR,
	CPSW_ALE_VLAN,
	CPSW_ALE_VLAN_ADDR,
};

enum cpsw_ale_iter_action {
	CPSW_ITER_NEXT = 0,
	CPSW_ITER_UPDATE,
	CPSW_ITER_RET,
	CPSW_ITER_UPDATE_RET,
};

struct cpsw_ale_entry {
	enum cpsw_ale_entry_type	type;
	unsigned char			addr[CPSW_ADDR_LEN];
	int				vlan_id;
	union {
		struct {
			enum {
				CPSW_ALE_UCAST_PERSISTANT = 0,
				CPSW_ALE_UCAST_UNTOUCHED,
				CPSW_ALE_UCAST_OUI,
				CPSW_ALE_UCAST_TOUCHED
			} ucast_type;
			int		secure;
			int		blocked;
			int		port_num;
		} ucast;
		struct {
			enum {
				CPSW_ALE_MCAST_FORWARD = 0,
				CPSW_ALE_MCAST_BLOCK_LEARN_FORWARD,
				CPSW_ALE_MCAST_FORWARD_LEARN,
				CPSW_ALE_MCAST_FORWARD_2
			} mcast_state;
			int		super;
			int		port_mask;
		} mcast;
              struct {
                     int mem_list;
                     int unreg_mcast_flood_mask;
                     int reg_mcast_flood_mask;
                     int force_untagged_egress;
              }vlan;
	} u;
};


/*********************************************************************
 * ALE Primitives - Helper routines to manipulate ALE table entries
 *********************************************************************/

static void cpsw_ale_dump_entry(PCPSW3G_ADAPTER pAdapter, int idx, struct cpsw_ale_entry *entry) {

	static const WCHAR* str_type[] = {
		L"free", L"addr", L"vlan", L"vlan+addr"};
	static const WCHAR* str_mcast_state[] = {
		L"f", L"blf", L"lf", L"f"};
	static const WCHAR* str_ucast_type[] = {
		L"persistant", L"untouched", L"oui", L"touched"};

    UNREFERENCED_PARAMETER(pAdapter);

       if(entry->type == CPSW_ALE_FREE) return;

	RETAILMSG(TRUE,  (L"CPSW_ALE_ENTRY[%d] - Type: %s, Addr: %02x:%02x:%02x:%02x:%02x:%02x, Vlan: %d\r\n",
			idx, str_type[entry->type],
			entry->addr[0], entry->addr[1], entry->addr[2],
			entry->addr[3], entry->addr[4], entry->addr[5],
			entry->vlan_id));

       if(entry->type == CPSW_ALE_VLAN){
		RETAILMSG(TRUE,    (L"    Vlan - Member: %x, Unreg flood mask: %x, Reg flood mask: %x, Force untagged: %x \r\n",
				entry->u.vlan.mem_list,
				entry->u.vlan.unreg_mcast_flood_mask,
				entry->u.vlan.reg_mcast_flood_mask,
				entry->u.vlan.force_untagged_egress));
       } else if (CPSW_MCAST(entry->addr)) {
		RETAILMSG(TRUE,   (L"    Multicast - State: %s, Port Mask: %x%s\r\n",
				str_mcast_state[entry->u.mcast.mcast_state],
				entry->u.mcast.port_mask,
				entry->u.mcast.super ? L" Super" : L""));
	} else {
		RETAILMSG(TRUE,  (L"    Unicast - Type: %s, Port: %d%s%s\r\n",
				str_ucast_type[entry->u.ucast.ucast_type],
				entry->u.ucast.port_num,
				entry->u.ucast.secure ? L" Secure" : L"",
				entry->u.ucast.blocked ? L" Blocked" : L""));
	}
}

/* Extract a field from a 68-bit (3x32-bit) ALE entry */
static __inline int
cpsw_ale_get_field(u32* val, u32 shift, u32 bits) {
	int idx, ret;

	ASSERT ((bits <= 30) && ((bits+shift) <= CPSW_ALE_ENTRY_BITS));

	idx    = shift / 32;
	shift -= idx * 32;

	/* This doesnt handle fields that straddle word boundaries */
	ASSERT ((shift + bits) <= 32);

	ret = (val[CPSW_ALE_ENTRY_WORDS - 1 - idx] >> shift) &
		CPSW_ALE_BITMASK(bits);

	return ret;
}

/* Insert a field into a 68-bit (3x32-bit) ALE entry */
static __inline int
cpsw_ale_set_field(u32* val, u32 shift, u32 bits, u32 fval) {
	int idx;

	ASSERT ((bits <= 30) && ((bits+shift) <= CPSW_ALE_ENTRY_BITS));

	fval &= CPSW_ALE_BITMASK(bits);

	idx    = shift / 32;
	shift -= idx * 32;

	/* This doesnt handle fvals that straddle word boundaries */
	ASSERT ((shift + bits) <= 32);

	val[CPSW_ALE_ENTRY_WORDS - 1 - idx] &= ~(CPSW_ALE_BITMASK(bits) << shift);
	val[CPSW_ALE_ENTRY_WORDS - 1 - idx] |=  (fval << shift);

	return 0;
}

/* Unpack an encoded 68-bit ALE entry into a more convenient struct */
static __inline void
cpsw_ale_unpack(struct cpsw_ale_entry* entry, u32* val) {
	NdisZeroMemory(entry, sizeof(*entry));
	
	entry->type = CPSW_ALE_GET_FIELD(val, ENTRY_TYPE);

	if (entry->type == CPSW_ALE_FREE)
		return;

	if (entry->type == CPSW_ALE_VLAN || entry->type == CPSW_ALE_VLAN_ADDR)
		entry->vlan_id = CPSW_ALE_GET_FIELD(val, VLAN_ID);

	if (entry->type == CPSW_ALE_ADDR || entry->type == CPSW_ALE_VLAN_ADDR) {
		int i;

		for (i = 0; i < CPSW_ADDR_LEN; i++)
			entry->addr[i] = (unsigned char)cpsw_ale_get_field(val, 40 - 8*i, 8);

		if (CPSW_MCAST(entry->addr)) {
			entry->u.mcast.port_mask   = CPSW_ALE_GET_FIELD(val, PORT_MASK);
			entry->u.mcast.super	   = CPSW_ALE_GET_FIELD(val, SUPER);
			entry->u.mcast.mcast_state = CPSW_ALE_GET_FIELD(val, MCAST_STATE);
		} else {
			entry->u.ucast.port_num	   = CPSW_ALE_GET_FIELD(val, PORT_NUM);
			entry->u.ucast.secure	   = CPSW_ALE_GET_FIELD(val, SECURE);
			entry->u.ucast.blocked	   = CPSW_ALE_GET_FIELD(val, BLOCKED);
			entry->u.ucast.ucast_type  = CPSW_ALE_GET_FIELD(val, UCAST_TYPE);
		}
	}
       if(entry->type == CPSW_ALE_VLAN){
            entry->u.vlan.mem_list = CPSW_ALE_GET_FIELD(val, VLAN_MEM_LIST);
            entry->u.vlan.unreg_mcast_flood_mask = CPSW_ALE_GET_FIELD(val, VLAN_UNREG_FLOOD);
            entry->u.vlan.reg_mcast_flood_mask = CPSW_ALE_GET_FIELD(val, VLAN_REG_FLOOD);
            entry->u.vlan.force_untagged_egress = CPSW_ALE_GET_FIELD(val, VLAN_FORCE_UNTAG_EGRESS);
       }
}

/* Pack an ALE data structure into an encoded 68-bit entry */
static __inline void
cpsw_ale_pack(struct cpsw_ale_entry* entry, u32* val) {
	NdisZeroMemory(val, sizeof(u32) * 3);

	CPSW_ALE_SET_FIELD(val, ENTRY_TYPE, entry->type);

	if (entry->type == CPSW_ALE_VLAN || entry->type == CPSW_ALE_VLAN_ADDR)
		CPSW_ALE_SET_FIELD(val, VLAN_ID, entry->vlan_id);

	if (entry->type == CPSW_ALE_ADDR || entry->type == CPSW_ALE_VLAN_ADDR) {
		int i;

		for (i = 0; i < CPSW_ADDR_LEN; i++)
			cpsw_ale_set_field(val, 40 - 8*i, 8, entry->addr[i]);

		if (CPSW_MCAST(entry->addr)) {
			CPSW_ALE_SET_FIELD(val, PORT_MASK,   entry->u.mcast.port_mask);
			CPSW_ALE_SET_FIELD(val, MCAST_STATE, entry->u.mcast.mcast_state);
			CPSW_ALE_SET_FIELD(val, SUPER,       entry->u.mcast.super);
		} else {
			CPSW_ALE_SET_FIELD(val, PORT_NUM,    entry->u.ucast.port_num);
			CPSW_ALE_SET_FIELD(val, SECURE,      entry->u.ucast.secure);
			CPSW_ALE_SET_FIELD(val, BLOCKED,     entry->u.ucast.blocked);
			CPSW_ALE_SET_FIELD(val, UCAST_TYPE,  entry->u.ucast.ucast_type);
		}
	}
       if(entry->type == CPSW_ALE_VLAN){
            CPSW_ALE_SET_FIELD(val, VLAN_MEM_LIST, entry->u.vlan.mem_list);
            CPSW_ALE_SET_FIELD(val, VLAN_UNREG_FLOOD, entry->u.vlan.unreg_mcast_flood_mask);
            CPSW_ALE_SET_FIELD(val, VLAN_REG_FLOOD, entry->u.vlan.reg_mcast_flood_mask);
            CPSW_ALE_SET_FIELD(val, VLAN_FORCE_UNTAG_EGRESS, entry->u.vlan.force_untagged_egress);
       }
}

/* Read and unpack a hardware ALE entry at the specified idx */
static int cpsw_ale_read(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry* entry) {
	u32 val[3];
	int i;

	if (idx > CPSW_ALE_TABLE_SIZE) {
		DEBUGMSG(DBG_ERR, (L"ALE table overflow on read\r\n"));
		return NDIS_STATUS_NOT_RECOGNIZED;
	}

	pAdapter->pCpsw3gRegsBase->ALE_TblCtl= idx;

	for (i = 0; i < 3; i++)
		val[i] = pAdapter->pCpsw3gRegsBase->ALE_Tbl[i];

	cpsw_ale_unpack(entry, val);
	return 0;
}

/* Pack and write a hardware ALE entry at the specified idx */
static int cpsw_ale_write(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry* entry) {
	u32 val[3];
	int i;

	if (idx > CPSW_ALE_TABLE_SIZE) {
		DEBUGMSG(DBG_ERR, (L"ALE table overflow on write\r\n"));
		return NDIS_STATUS_NOT_RECOGNIZED;
	}

	cpsw_ale_pack(entry, val);

	for (i = 0; i < 3; i++)
		pAdapter->pCpsw3gRegsBase->ALE_Tbl[i] = val[i];

//    RETAILMSG(TRUE, (L"  ++ ALE entry[%d]: val[0]=%x, val[1]=%x, val[2]=%x\r\n", idx, val[0], val[1], val[2]));

	pAdapter->pCpsw3gRegsBase->ALE_TblCtl = idx | CPSW_ALE_TABLE_WRITE;

	return 0;
}

/* 
 * General purpose iterator
 * 	- call the specified function for every ALE entry
 * 	- helper function (match) can return one of:
 *	    CPSW_ITER_NEXT       - continue with the next entry
 *	    CPSW_ITER_UPDATE     - writeback and continue
 *	    CPSW_ITER_RET        - terminate iterator, return idx
 *          CPSW_ITER_UPDATE_RET - writeback and return idx
 */
static int cpsw_ale_iterate(PCPSW3G_ADAPTER pAdapter, void* arg,
		int (*match)(PCPSW3G_ADAPTER pAdapter,
			     int			idx,
			     struct cpsw_ale_entry	*entry,
			     void			*arg)) {
	int			idx;
	struct cpsw_ale_entry	entry;
	int			ret, code;

	for (idx = 0; idx < CPSW_ALE_TABLE_SIZE; idx++) {
		ret = cpsw_ale_read(pAdapter, idx, &entry);
		if (ret < 0) {
			DEBUGMSG(DBG_ERR, (L"Failed to iterate through ALE entry %d\r\n", idx));
			return ret;
		}

		code = match(pAdapter, idx, &entry, arg);

		if (code < 0)
			return code;
		
		if (code == CPSW_ITER_UPDATE ||
		    code == CPSW_ITER_UPDATE_RET) {
			ret = cpsw_ale_write(pAdapter, idx, &entry);
			if (ret < 0)
				return ret;
		}

		if (code == CPSW_ITER_RET ||
		    code == CPSW_ITER_UPDATE_RET)
			return idx;
	}

	return 0;
}

/* Find an ALE entry that matches a specified MAC address */
static int __cpsw_ale_match_addr(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {

	unsigned char* addr = (unsigned char*)arg;
	int islast = (idx == (CPSW_ALE_TABLE_SIZE - 1));
	int ret = islast ? NDIS_STATUS_FAILURE : CPSW_ITER_NEXT;

       UNREFERENCED_PARAMETER(pAdapter);

	if (entry->type != CPSW_ALE_ADDR &&
	    entry->type != CPSW_ALE_VLAN_ADDR)
		return ret;

	if (!NdisEqualMemory(entry->addr, addr, CPSW_ADDR_LEN))
		return ret;

	return CPSW_ITER_RET;
}

static int cpsw_ale_match_addr(PCPSW3G_ADAPTER pAdapter,
		unsigned char* addr) {

	return cpsw_ale_iterate(pAdapter, addr,
			__cpsw_ale_match_addr);
}

/* Find an ALE entry that matches a specified MAC address and VLAN */
static int __cpsw_ale_match_vlan_addr(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {

	struct cpsw_ale_entry* match = arg;
	int islast = (idx == (CPSW_ALE_TABLE_SIZE - 1));
	int ret = islast ? NDIS_STATUS_FAILURE : CPSW_ITER_NEXT;

       UNREFERENCED_PARAMETER(pAdapter);

	if (entry->type != CPSW_ALE_VLAN_ADDR)
		return ret;

	if (entry->vlan_id != match->vlan_id)
		return ret;

	if (!NdisEqualMemory(entry->addr, match->addr, CPSW_ADDR_LEN))
		return ret;

	return CPSW_ITER_RET;
}

static int cpsw_ale_match_vlan_addr(PCPSW3G_ADAPTER pAdapter,
		unsigned int vlan_id,
		unsigned char* addr) {
	struct cpsw_ale_entry match;

	NdisMoveMemory(&match.addr, addr, CPSW_ADDR_LEN);
	match.vlan_id = vlan_id;
       match.type = CPSW_ALE_VLAN_ADDR;
	return cpsw_ale_iterate(pAdapter, &match,
			__cpsw_ale_match_vlan_addr);
}

/* Find an ALE entry that matches a specified VLAN ID */
static int __cpsw_ale_match_vlan(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {

	struct cpsw_ale_entry* match = arg;
	int islast = (idx == (CPSW_ALE_TABLE_SIZE - 1));
	int ret = islast ? NDIS_STATUS_FAILURE : CPSW_ITER_NEXT;

       UNREFERENCED_PARAMETER(pAdapter);

	if (entry->type != CPSW_ALE_VLAN)
		return ret;

	if (entry->vlan_id != match->vlan_id)
		return ret;

	return CPSW_ITER_RET;
}

static int cpsw_ale_match_vlan(PCPSW3G_ADAPTER pAdapter,
		unsigned int vlan_id) {
	struct cpsw_ale_entry match;

	match.vlan_id = vlan_id;
       match.type = CPSW_ALE_VLAN;
	return cpsw_ale_iterate(pAdapter, &match,
			__cpsw_ale_match_vlan);
}


/* Find a free ALE table idx */
static int __cpsw_ale_match_free(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {

	int islast = (idx == (CPSW_ALE_TABLE_SIZE - 1));
	int ret = islast ? NDIS_STATUS_FAILURE : CPSW_ITER_NEXT;

       UNREFERENCED_PARAMETER(pAdapter);
       UNREFERENCED_PARAMETER(arg);

	if (entry->type != CPSW_ALE_FREE)
		return ret;

	return CPSW_ITER_RET;
}

static int cpsw_ale_find_free(PCPSW3G_ADAPTER pAdapter) {
	return cpsw_ale_iterate(pAdapter, NULL,
			__cpsw_ale_match_free);
}

/* Flush ALE entries for specified ports */
static int __cpsw_ale_flush(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {
	int port_mask = (int)arg;
	int ret = CPSW_ITER_NEXT;
    
       UNREFERENCED_PARAMETER(pAdapter);
       UNREFERENCED_PARAMETER(idx);

	if (entry->type == CPSW_ALE_FREE)
		return CPSW_ITER_NEXT;

	if (entry->type != CPSW_ALE_ADDR &&
	    entry->type != CPSW_ALE_VLAN_ADDR)
		return CPSW_ITER_NEXT;

	if (CPSW_MCAST(entry->addr)) {
		if (entry->u.mcast.port_mask & port_mask) {
			entry->u.mcast.port_mask &= ~port_mask;
			if (entry->u.mcast.port_mask ==
					(1 << CPSW_UNIT_SWITCH))
				entry->type = CPSW_ALE_FREE;
			ret = CPSW_ITER_UPDATE;
		}
	} else {
		if (port_mask & (1 << entry->u.ucast.port_num)) {
			entry->type = CPSW_ALE_FREE;
			ret = CPSW_ITER_UPDATE;
		}
	}

	return ret;
}


/* Flush ALE mcast entries  */
static int __cpsw_ale_match_mcast(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {
	int ret = CPSW_ITER_NEXT;

       UNREFERENCED_PARAMETER(pAdapter);
       UNREFERENCED_PARAMETER(arg);
       UNREFERENCED_PARAMETER(idx);
      
	if (entry->type == CPSW_ALE_FREE)
		return ret;

	if (CPSW_MCAST(entry->addr) && !CPSW_BCAST(entry->addr)) {
		entry->type = CPSW_ALE_FREE;
		ret = CPSW_ITER_UPDATE;
	} else {
		return ret;
	}

	return ret;
}

static int cpsw_ale_flush(PCPSW3G_ADAPTER pAdapter, int port_mask) {
	return cpsw_ale_iterate(pAdapter, (void*)port_mask,
			__cpsw_ale_flush);
}

/* Dump the entire ALE table */
static int __cpsw_ale_dump(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {

       UNREFERENCED_PARAMETER(arg);
	cpsw_ale_dump_entry(pAdapter, idx, entry);
	return CPSW_ITER_NEXT;
}

int cpsw_ale_dump(PCPSW3G_ADAPTER pAdapter) {
	return cpsw_ale_iterate(pAdapter, NULL,
			__cpsw_ale_dump);
}

/* Find an ageable entry suitable for deletion */
static int __cpsw_ale_match_ageable(PCPSW3G_ADAPTER pAdapter, int idx,
		struct cpsw_ale_entry *entry, void* arg) {

	int islast = (idx == (CPSW_ALE_TABLE_SIZE - 1));
	int ret = islast ? NDIS_STATUS_FAILURE : CPSW_ITER_NEXT;
       UNREFERENCED_PARAMETER(pAdapter);
       UNREFERENCED_PARAMETER(arg);

	if (entry->type == CPSW_ALE_FREE ||
	    entry->type == CPSW_ALE_VLAN)
		return ret;

	if (CPSW_MCAST(entry->addr))
		return ret;

	if (entry->u.ucast.ucast_type == CPSW_ALE_UCAST_PERSISTANT ||
	    entry->u.ucast.ucast_type == CPSW_ALE_UCAST_OUI)
		return ret;

	return CPSW_ITER_RET;
}

static int cpsw_ale_find_ageable(PCPSW3G_ADAPTER pAdapter) {
	return cpsw_ale_iterate(pAdapter, NULL,
			__cpsw_ale_match_ageable);
}

int cpsw_ale_add_vlan_entry(PCPSW3G_ADAPTER pAdapter, 
		int vlan_id, int member, int force_untag, 
		int reg_mcast_flood_mask, int unreg_mcast_flood_mask) 
{
	int			idx, ret;
	struct cpsw_ale_entry	entry;

       NdisAcquireSpinLock(&pAdapter->Lock); 

	/* 
	 * Construct a Vlan  ALE entry 
	 */
	NdisZeroMemory(&entry, sizeof(entry));
	entry.type = CPSW_ALE_VLAN;
       entry.vlan_id = vlan_id;
	entry.u.vlan.mem_list = member;
	entry.u.vlan.unreg_mcast_flood_mask = unreg_mcast_flood_mask;
	entry.u.vlan.reg_mcast_flood_mask = reg_mcast_flood_mask;
	entry.u.vlan.force_untagged_egress = force_untag;

	idx = cpsw_ale_match_vlan(pAdapter, vlan_id);
	if (idx < 0)
		idx = cpsw_ale_find_free(pAdapter);
	if (idx < 0)
		idx = cpsw_ale_find_ageable(pAdapter);
	if (idx < 0) {
		/* Give up... we really need that ALE entry */
        	NdisReleaseSpinLock(&pAdapter->Lock);
		RETAILMSG(TRUE, (L"failed to find free ALE entry\r\n"));
		cpsw_ale_dump(pAdapter);
		return NDIS_STATUS_RESOURCES;
	}

	/* Write the ALE entry */
	ret = cpsw_ale_write(pAdapter, idx, &entry);

	NdisReleaseSpinLock(&pAdapter->Lock);

	return (ret < 0) ? ret : idx;

}
int cpsw_ale_del_vlan_entry(PCPSW3G_ADAPTER pAdapter, int vlan_id) {
	int			idx, ret;
	struct cpsw_ale_entry	entry;

       NdisAcquireSpinLock(&pAdapter->Lock); 

	ret = NDIS_STATUS_FAILURE;
	idx = cpsw_ale_match_vlan(pAdapter, vlan_id);
	if (idx < 0)
		goto ret_unlock;

	ret = cpsw_ale_read(pAdapter, idx, &entry);
	if (ret < 0)
		goto ret_unlock;

	entry.type = CPSW_ALE_FREE;

	ret = cpsw_ale_write(pAdapter, idx, &entry);

ret_unlock:
	NdisReleaseSpinLock(&pAdapter->Lock);

	return (ret < 0) ? ret : idx;
}

int cpsw_ale_add_vlan_ucast_entry(PCPSW3G_ADAPTER pAdapter,
		int vlan_id, unsigned char* mac, int port) 
{
	int			idx, ret;
	struct cpsw_ale_entry	entry;

	if (CPSW_MCAST(mac)) {
		RETAILMSG(TRUE, (L"mcast address (%02x:%02x:%02x:%02x:%02x:%02x)add requested in ucast entry\r\n",
				mac[0], mac[1], mac[2], mac[3],
				mac[4], mac[5]));
		return NDIS_STATUS_NOT_RECOGNIZED;
	}
       NdisAcquireSpinLock(&pAdapter->Lock); 

	/* 
	 * Construct a Vlan unicast ALE entry to redirect this
	 * port's MAC address to the host port
	 */
	NdisZeroMemory(&entry, sizeof(entry));
	NdisMoveMemory(entry.addr, mac, CPSW_ADDR_LEN);
	entry.type = CPSW_ALE_VLAN_ADDR;
       entry.vlan_id = vlan_id;
	entry.u.ucast.ucast_type	= CPSW_ALE_UCAST_PERSISTANT;
	entry.u.ucast.secure		= 1;
	entry.u.ucast.blocked		= 0;
	entry.u.ucast.port_num		= port;

	idx = cpsw_ale_match_vlan_addr(pAdapter, vlan_id, mac);
	if (idx < 0)
		idx = cpsw_ale_find_free(pAdapter);
	if (idx < 0)
		idx = cpsw_ale_find_ageable(pAdapter);
	if (idx < 0) {
		/* Give up... we really need that ALE entry */
        	NdisReleaseSpinLock(&pAdapter->Lock);
		RETAILMSG(TRUE, (L"failed to find free ALE entry\r\n"));
		cpsw_ale_dump(pAdapter);
		return NDIS_STATUS_RESOURCES;
	}

	/* Write the ALE entry */
	ret = cpsw_ale_write(pAdapter, idx, &entry);

	NdisReleaseSpinLock(&pAdapter->Lock);

	return (ret < 0) ? ret : idx;
}
int cpsw_ale_add_ucast_entry(PCPSW3G_ADAPTER pAdapter,
		unsigned char* mac, int port) {
	int			idx, ret;
	struct cpsw_ale_entry	entry;

	if (CPSW_MCAST(mac)) {
		RETAILMSG(TRUE, (L"mcast address (%02x:%02x:%02x:%02x:%02x:%02x)add requested in ucast entry\r\n",
				mac[0], mac[1], mac[2], mac[3],
				mac[4], mac[5]));
		return NDIS_STATUS_NOT_RECOGNIZED;
	}

       NdisAcquireSpinLock(&pAdapter->Lock); 

	/* 
	 * Construct a unicast ALE entry to redirect this
	 * port's MAC address to the host port
	 */
	NdisZeroMemory(&entry, sizeof(entry));
	NdisMoveMemory(entry.addr, mac, CPSW_ADDR_LEN);
	entry.type = CPSW_ALE_ADDR;
	entry.u.ucast.ucast_type	= CPSW_ALE_UCAST_PERSISTANT;
	entry.u.ucast.secure		= 1;
	entry.u.ucast.blocked		= 0;
	entry.u.ucast.port_num		= port;

	idx = cpsw_ale_match_addr(pAdapter, mac);
	if (idx < 0)
		idx = cpsw_ale_find_free(pAdapter);
	if (idx < 0)
		idx = cpsw_ale_find_ageable(pAdapter);
	if (idx < 0) {
		/* Give up... we really need that ALE entry */
        	NdisReleaseSpinLock(&pAdapter->Lock);
		RETAILMSG(TRUE, (L"failed to find free ALE entry\r\n"));
		cpsw_ale_dump(pAdapter);
		return NDIS_STATUS_RESOURCES;
	}

	/* Write the ALE entry */
	ret = cpsw_ale_write(pAdapter, idx, &entry);

	NdisReleaseSpinLock(&pAdapter->Lock);

	return (ret < 0) ? ret : idx;
}

int cpsw_ale_del_ucast_entry(PCPSW3G_ADAPTER pAdapter,
		unsigned char* mac, int port) {
	int			idx, ret;
	struct cpsw_ale_entry	entry;

       UNREFERENCED_PARAMETER(port);
	if (CPSW_MCAST(mac)) {
		RETAILMSG(TRUE, (L"mcast address (%02x:%02x:%02x:%02x:%02x:%02x) del requested in ucast entry\r\n",
				mac[0], mac[1], mac[2], mac[3],
				mac[4], mac[5]));
		return NDIS_STATUS_NOT_RECOGNIZED;
	}

       NdisAcquireSpinLock(&pAdapter->Lock); 

	ret = NDIS_STATUS_FAILURE;
	idx = cpsw_ale_match_addr(pAdapter, mac);
	if (idx < 0)
		goto ret_unlock;

	ret = cpsw_ale_read(pAdapter, idx, &entry);
	if (ret < 0)
		goto ret_unlock;

	entry.type = CPSW_ALE_FREE;

	ret = cpsw_ale_write(pAdapter, idx, &entry);

ret_unlock:
	NdisReleaseSpinLock(&pAdapter->Lock);

	return (ret < 0) ? ret : idx;
}

int cpsw_ale_add_mcast_entry(PCPSW3G_ADAPTER pAdapter,
		unsigned char* mac, int vlan,  int port_mask) {
	int			idx, ret;
	struct cpsw_ale_entry	entry;

	if (!CPSW_MCAST(mac)) {
		RETAILMSG(TRUE, (L"mcast address (%02x:%02x:%02x:%02x:%02x:%02x)add requested in mcast entry\r\n",
				mac[0], mac[1], mac[2], mac[3],
				mac[4], mac[5]));
		return NDIS_STATUS_NOT_RECOGNIZED;
	}

       NdisAcquireSpinLock(&pAdapter->Lock); 

	/* 
	 * Construct a unicast ALE entry to redirect this
	 * port's MAC address to the host port
	 */
	NdisZeroMemory(&entry, sizeof(entry));
	NdisMoveMemory(entry.addr, mac, CPSW_ADDR_LEN);
       entry.vlan_id = vlan;
       if(vlan)
           entry.type = CPSW_ALE_VLAN_ADDR;
       else
           entry.type = CPSW_ALE_ADDR;
	entry.u.mcast.mcast_state	= CPSW_ALE_MCAST_FORWARD_2;
	entry.u.mcast.port_mask		= port_mask;

	idx = cpsw_ale_match_addr(pAdapter, mac);
	if (idx >= 0) {
		ret = cpsw_ale_read(pAdapter, idx, &entry);
		if (entry.u.mcast.mcast_state != CPSW_ALE_MCAST_FORWARD &&
		    entry.u.mcast.mcast_state != CPSW_ALE_MCAST_FORWARD_2) {
			DEBUGMSG(DBG_WARN, (L"unrecognized mcast ALE entry found\r\n"));
			cpsw_ale_dump_entry(pAdapter, idx, &entry);
		}
		entry.u.mcast.mcast_state	 = CPSW_ALE_MCAST_FORWARD_2;
		entry.u.mcast.port_mask		|= port_mask;
	}

	if (idx < 0)
		idx = cpsw_ale_find_free(pAdapter);
	if (idx < 0)
		idx = cpsw_ale_find_ageable(pAdapter);
	if (idx < 0) {
		/* Give up... we really need that ALE entry */
        	NdisReleaseSpinLock(&pAdapter->Lock);
		DEBUGMSG(DBG_ERR, (L"failed to find free ALE entry\r\n"));
		cpsw_ale_dump(pAdapter);
		return NDIS_STATUS_RESOURCES;
	}

	/* Write the ALE entry */
	ret = cpsw_ale_write(pAdapter, idx, &entry);

	NdisReleaseSpinLock(&pAdapter->Lock);

	return (ret < 0) ? ret : idx;
}

int cpsw_ale_del_mcast_entry(PCPSW3G_ADAPTER pAdapter,
		unsigned char* mac, int port_mask) {
	int			idx, ret;
	struct cpsw_ale_entry	entry;

	if (!CPSW_MCAST(mac)) {
		RETAILMSG(TRUE, (L"ucast address (%02x:%02x:%02x:%02x:%02x:%02x) del requested in mcast entry\r\n",
				mac[0], mac[1], mac[2], mac[3],
				mac[4], mac[5]));
		return NDIS_STATUS_NOT_RECOGNIZED;
	}

       /* Acquire the Send lock */
       NdisAcquireSpinLock(&pAdapter->Lock); 

	ret = NDIS_STATUS_FAILURE;
	idx = cpsw_ale_match_addr(pAdapter, mac);
	if (idx < 0)
		goto ret_unlock;

	ret = cpsw_ale_read(pAdapter, idx, &entry);
	if (ret < 0)
		goto ret_unlock;

	if (entry.u.mcast.mcast_state != CPSW_ALE_MCAST_FORWARD &&
			entry.u.mcast.mcast_state != CPSW_ALE_MCAST_FORWARD_2) {
		DEBUGMSG(DBG_WARN, (L"unrecognized mcast ALE entry found\r\n"));
		cpsw_ale_dump_entry(pAdapter, idx, &entry);
	}
	entry.u.mcast.mcast_state	 = CPSW_ALE_MCAST_FORWARD_2;
	entry.u.mcast.port_mask		&= ~port_mask;

	if (entry.u.mcast.port_mask == (1 << CPSW_UNIT_SWITCH))
		entry.type = CPSW_ALE_FREE;

	ret = cpsw_ale_write(pAdapter, idx, &entry);

ret_unlock:
	NdisReleaseSpinLock(&pAdapter->Lock);

	return (ret < 0) ? ret : idx;
}

int cpsw_ale_flush_mcast_entry(PCPSW3G_ADAPTER pAdapter)
{
    	return cpsw_ale_iterate(pAdapter, NULL,  __cpsw_ale_match_mcast);
}
