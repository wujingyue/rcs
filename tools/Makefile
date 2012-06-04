LEVEL = ..

include $(LEVEL)/Makefile.common

# TODO: file names in the source directory needn't prefix rcs_.
Scripts = rcs_config.py rcs_utils.py rcs_dump_ids.py rcs_locate_src.py rcs_test_aa.py

all-local:: $(addprefix $(ToolDir)/,$(Scripts))

$(ToolDir)/rcs_config.py:: $(PROJ_OBJ_DIR)/rcs_config.py.in $(ToolDir)/.dir
	$(Echo) Building rcs_config.py
	$(Verb) $(SED) 's/@BuildMode@/$(BuildMode)/' < $< > $@

$(ToolDir)/rcs_utils.py:: $(PROJ_SRC_DIR)/rcs_utils.py $(ToolDir)/.dir
	$(Echo) Building rcs_utils.py
	$(Verb) $(CP) -f $< $@

$(ToolDir)/%.py:: $(PROJ_SRC_DIR)/%.py $(ToolDir)/.dir
	$(Echo) Building $*.py
	$(Verb) $(CP) -f $< $@
	$(Verb) chmod +x $@

clean-local::
	$(Verb) $(RM) -f $(addprefix $(ToolDir)/,$(Scripts))

install-local:: all-local
	$(Verb) for script in $(Scripts); do \
		$(Echo) Installing $$script
		$(Verb) $(MKDIR) $(DESTDIR)$(PROJ_bindir)
		$(Verb) $(Install) $(ToolDir)/$$script $(DESTDIR)$(PROJ_bindir)
	done
