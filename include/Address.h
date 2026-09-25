#pragma once

/**
 * @brief   Global EEPROM address allocator.
 *
 * Provides a shared EEPROM address counter.
 * Each object reserves its EEPROM address automatically during construction.
 */
inline int eeprom = 0;