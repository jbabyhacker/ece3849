## THIS IS A GENERATED FILE -- DO NOT EDIT
.configuro: .libraries,em3 linker.cmd package/cfg/app_pem3.oem3

linker.cmd: package/cfg/app_pem3.xdl
	$(SED) 's"^\"\(package/cfg/app_pem3cfg.cmd\)\"$""\"C:/Users/Jason Klein/Documents/GitHub/ece3849/ece3849b14_lab2_jasoneklein_nwotero/.config/xconfig_app/\1\""' package/cfg/app_pem3.xdl > $@
