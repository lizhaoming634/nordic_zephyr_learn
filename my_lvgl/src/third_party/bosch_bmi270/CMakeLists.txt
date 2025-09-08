# 把 Bosch 源码做成一个“有名字”的 Zephyr 库：bmi270_bosch
zephyr_library_named(bmi270_bosch)

zephyr_library_sources(
  bmi2.c
  bmi270.c
  common.c        # 若没用到 common.c/h，可删掉此行并移除文件
)

# 这个库的 PUBLIC 头文件路径（会传递给依赖它的目标）
target_include_directories(bmi270_bosch PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)
