/**
 * @file qret/target/sc_ls_fixed_v0/exception.h
 * @brief SC_LS_FIXED_V0 exception class.
 */

#ifndef QRET_TARGET_SC_LS_FIXED_V0_EXCEPTION_H
#define QRET_TARGET_SC_LS_FIXED_V0_EXCEPTION_H

#include "qret/exception.h"
#include "qret/qret_export.h"

namespace qret::sc_ls_fixed_v0 {
class QRET_EXPORT ScLsError : public QRETError {
public:
    using QRETError::QRETError;
};
}  // namespace qret::sc_ls_fixed_v0

#endif  // QRET_TARGET_SC_LS_FIXED_V0_EXCEPTION_H
