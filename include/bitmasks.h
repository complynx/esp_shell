#ifndef __BITMASKS_H__
#define __BITMASKS_H__

#define MASK_BIT(N) (1<<(N))

#define MASK_HAS(mask,bits) ((mask)&(bits))
#define MASK_HASNOT(mask,bits) (~((mask)&(bits)))
#define MASK_SET(mask,bits) ((mask)|=(bits))
#define MASK_UNSET(mask,bits) ((mask)&=(~(bits)))

#define MASK_HASN(mask,bits) ((mask)& MASK_BIT(bits))
#define MASK_HASNOTN(mask,bits) (~((mask)& MASK_BIT(bits)))
#define MASK_SETN(mask,bits) ((mask)|= MASK_BIT(bits))
#define MASK_UNSETN(mask,bits) ((mask)&=(~ MASK_BIT(bits)))

#endif//__BITMASKS_H__
