#include "JWEMSingleton.h"
#include <mutex>

Singleton * Singleton::instance;


std::mutex s_mutex;
