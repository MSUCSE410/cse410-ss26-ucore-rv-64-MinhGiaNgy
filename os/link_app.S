    .align 4
    .section .data
    .global _app_num
_app_num:
    .quad 3
    .quad app_0_start
    .quad app_1_start
    .quad app_2_start
    .quad app_2_end

    .global _app_names
_app_names:
   .string "ch2b_exit"
   .string "ch2b_hello_world"
   .string "ch2b_power"

    .section .data.app0
    .global app_0_start
app_0_start:
    .incbin "./user/target/bin/ch2b_exit"

    .section .data.app1
    .global app_1_start
app_1_start:
    .incbin "./user/target/bin/ch2b_hello_world"

    .section .data.app2
    .global app_2_start
app_2_start:
    .incbin "./user/target/bin/ch2b_power"
app_2_end:

