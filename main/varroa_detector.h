#pragma once

#include "dl_detect_base.hpp"

namespace varroadet_detect {
    class VarroaDet : public dl::detect::DetectImpl {
    public:
        VarroaDet();
    };
}  // namespace varroadet_detect

// Public wrapper that provides run()
class VarroaDetDetect : public dl::detect::DetectWrapper {
public:
    VarroaDetDetect();
    
protected:
    void load_model() override;  // Required
};
