# 业务 + HAL
target_sources(app PRIVATE
  bmi270_hal.c
  steps_service.c
)

# 业务自己的头
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# ★ 显式依赖第三方库（从而继承它的 PUBLIC include 路径 & 实现）
target_link_libraries(app PRIVATE bmi270_bosch)
