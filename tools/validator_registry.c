#include "entity.h"

void validator_registry_init(void) {
    entity_registry_init();
    entity_register_builtin_stubs();
}
