extern "C" int bootboot_main() {
  asm("cli");
  asm("hlt");
  return 0;
}
