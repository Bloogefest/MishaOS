from elftools.dwarf.descriptions import describe_form_class
from elftools.elf.elffile import ELFFile

if __name__ == '__main__':
    with open('build/mishaos.debug', 'rb') as f:
        elffile = ELFFile(f)

        if not elffile.has_dwarf_info():
            print('No DWARF info.')
            exit(-1)

        dwarfinfo = elffile.get_dwarf_info()

        with open('initrd/.funcs', 'wb') as out:
            for CU in dwarfinfo.iter_CUs():
                for DIE in CU.iter_DIEs():
                    try:
                        if DIE.tag == 'DW_TAG_subprogram':
                            lowpc = DIE.attributes['DW_AT_low_pc'].value
                            highpc_attr = DIE.attributes['DW_AT_high_pc']
                            highpc_attr_class = describe_form_class(highpc_attr.form)
                            if highpc_attr_class == 'address':
                                highpc = highpc_attr.value
                            elif highpc_attr_class == 'constant':
                                highpc = lowpc + highpc_attr.value
                            else:
                                print('Error: invalid DW_AT_high_pc class:', highpc_attr_class)
                                continue

                            value = DIE.attributes['DW_AT_name'].value
                            out.write((11 + len(value)).to_bytes(2, 'little'))
                            out.write(lowpc.to_bytes(4, 'little'))
                            out.write(highpc.to_bytes(4, 'little'))
                            out.write(value)
                            out.write(b'\x00')
                    except KeyError:
                        continue

            out.write(b'\x00\x00')
