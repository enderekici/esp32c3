- name: Write build info header
  run: |
    echo "#pragma once" > build_info.h
    echo "#define BUILD_COMMIT \"${{ steps.meta.outputs.commit }}\"" >> build_info.h
    echo "#define BUILD_DATE \"${{ steps.meta.outputs.build_date }}\"" >> build_info.h
    echo "#define BUILD_VERSION \"${{ steps.meta.outputs.version }}\"" >> build_info.h