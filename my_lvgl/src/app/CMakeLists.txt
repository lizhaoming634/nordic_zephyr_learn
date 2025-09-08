target_sources(app PRIVATE 
  backlight_ctrl.c
  key_ebtn.c
  ebtn.c
)

target_include_directories(app PRIVATE app)