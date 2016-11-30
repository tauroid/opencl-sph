#ifndef CONFIG_H_
#define CONFIG_H_

void load_config(const char * relativePath);
const char * get_config_section(const char * heading);
void unload_config();

#endif
