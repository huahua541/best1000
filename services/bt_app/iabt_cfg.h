/***
 * iabt_cfg.h
 */

#ifndef IABT_CFG_H
#define IABT_CFG_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct iabt_cfg_t{
    bool sniff;
    bool one_bring_two;
};
extern struct iabt_cfg_t iabt_cfg;

#ifdef __cplusplus
}
#endif
#endif /* IABT_H */
