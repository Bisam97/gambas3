ACLOCAL_AMFLAGS = -I m4 --install
SUBDIRS = gbc gbx . lib comp share man
EXTRA_DIST = TODO reconf tools spec README mime gb.*.h

install-exec-local:
	@rm -f $(srcdir)/../warnings.log
	
	@if test "x$(ROOT)" != "x"; then \
	 echo "[Installing with ROOT=$(ROOT)]"; \
	fi
	@if test "x$(DESTDIR)" != "x"; then \
	  echo "[Installing with DESTDIR=$(DESTDIR)]"; \
	  ROOT=$DESTDIR; \
	fi
	
	@echo "Installing runtime symbolic link"
	@$(LN_S) -f gbx$(GAMBAS_VERSION) $(DESTDIR)$(bindir)/gbr$(GAMBAS_VERSION) || true
	
	@if test x"$(XDG_UTILS)" != x; then \
	  echo "Registering Gambas executable mimetype"; \
	  xdg-icon-resource install --novendor --context mimetypes --size 256 $(srcdir)/mime/application-x-gambas3.png application-x-gambas3; \
	  xdg-icon-resource install --novendor --context mimetypes --size 48 $(srcdir)/mime/application-x-gambas3-48.png application-x-gambas3; \
	  xdg-mime install $(srcdir)/mime/application-x-gambas3.xml; \
	fi

	@echo "Installing 'gb' component..."
	@$(INSTALL) -d $(DESTDIR)$(gbdatadir)/info
	@$(INSTALL) lib/gb.component $(DESTDIR)$(gblibdir)
	@$(DESTDIR)$(bindir)/gbi$(GAMBAS_VERSION) -r $(DESTDIR)$(prefix) gb
	@rm -f $(DESTDIR)$(gblibdir)/gb.la
	@rm -f $(DESTDIR)$(gblibdir)/gb.so*

	@echo "Installing compiler tools..."
	@(cd $(srcdir)/tools; d=`pwd`; \
	  for p in gb*; do \
	    echo "Compiling $$p..."; cd $$d/$$p; \
	    $(DESTDIR)$(bindir)/gbc$(GAMBAS_VERSION) -ag -r $(DESTDIR)$(prefix); \
	    if test $$? -eq 0; then \
	      $(DESTDIR)$(bindir)/gba$(GAMBAS_VERSION); \
	      rm -rf .gambas; \
	      echo "Installing $$p..."; \
	      $(INSTALL) $$p.gambas $(DESTDIR)$(bindir); \
	      $(LN_S) -f $$p.gambas $(DESTDIR)$(bindir)/$$p || true; \
	    else \
	      echo "|| Unable to compile $$p" >> ../../../warnings.log; \
	    fi \
	  done)

uninstall-local:
	@echo "Uninstalling compiler tools..."
	@(cd $(srcdir)/tools; for p in gb*; do rm -f $(DESTDIR)$(bindir)/$$p.gambas $(DESTDIR)$(bindir)/$$p; done)

	@echo "Uninstalling 'gb' component..."
	@rm -f $(DESTDIR)$(gblibdir)/gb.component
	@rm -rf $(DESTDIR)$(gbdatadir)/info/gb.info
	@rm -rf $(DESTDIR)$(gbdatadir)/info/gb.list

	@if test x"$(XDG_UTILS)" != x; then \
	  echo "Unregistering Gambas executable mimetype"; \
	  xdg-mime uninstall $(srcdir)/mime/application-x-gambas3.xml; \
	  xdg-icon-resource uninstall --context mimetypes --size 256 application-x-gambas3; \
	fi
	
	@echo "Uninstalling runtime symbolic link"
	@rm -f $(DESTDIR)$(bindir)/gbr$(GAMBAS_VERSION)

	@rmdir $(DESTDIR)$(gbdatadir)/info >/dev/null 2>&1 || true;
	@rmdir $(DESTDIR)$(gbdatadir)/control >/dev/null 2>&1 || true;
	@rmdir $(DESTDIR)$(gbdatadir) >/dev/null 2>&1 || true;

dist-hook:
	@rm -f $(distdir)/trunk_version.h
	@(cd $(distdir)/tools; \
	rm -rf `find . -name ".gambas" -o -name ".action" -o -name ".lock" -o -name ".xvpics" -o -name "*~" -o -name "*.out"  -o -name "*.pot" -o -name "*.gambas" -o -name "core*" -o -name ".kdbg*" -o -name ".svn"`;)


