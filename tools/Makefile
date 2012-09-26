LEVEL = ..

include $(LEVEL)/Makefile.common

# TODO: file names in the source directory needn't prefix rcs_.
Scripts = rcs_utils.py rcs_dump_ids.py rcs_locate_src.py rcs_test_aa.py rcs_lookup_value.py

install-local::
	$(Verb) for script in $(Scripts) ; do \
		echo Installing $$script ; \
		$(MKDIR) $(DESTDIR)$(PROJ_bindir) ; \
		if [ "$$script" = "rcs_utils.py" ]; then \
			$(DataInstall) $(PROJ_SRC_DIR)/$$script $(DESTDIR)$(PROJ_bindir) ; \
		else \
			$(ScriptInstall) $(PROJ_SRC_DIR)/$$script $(DESTDIR)$(PROJ_bindir) ; \
		fi; \
	done

uninstall-local::
	$(Verb) $(RM) -f $(addprefix $(DESTDIR)$(PROJ_bindir)/,$(Scripts))
