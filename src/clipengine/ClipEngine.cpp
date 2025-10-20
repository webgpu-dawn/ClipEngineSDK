#include "ClipEngine.h"
#include "./util/Logger.h"

namespace ClipEngine {

const char* getVersion() {
    LOG_INFO("ClipEngine version : {}", "v1.0.0");
    return "ClipEngine v1.0.0";
}

} // namespace ClipEngine