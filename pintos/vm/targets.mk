vm_SRC = vm/vm.c          # 공통 API 진입점
vm_SRC += vm/uninit.c     # 미초기화 페이지
vm_SRC += vm/anon.c       # 익명 페이지
vm_SRC += vm/file.c       # 파일 매핑 페이지
vm_SRC += vm/inspect.c    # 채점용 검사 도구
