EXTRA_DIST = reconf src spec mime desktop template
SUBDIRS = . man

install-exec-local:
	@if test "x$(ROOT)" != "x"; then \
	 echo "[Installing with ROOT=$(ROOT)]"; \
	fi
	@if test "x$(DESTDIR)" != "x"; then \
	 echo "[Installing with DESTDIR=$(DESTDIR)]"; \
	 ROOT=$DESTDIR; \
	fi
	
	@echo "Installing the development environment..."
	@(cd $(srcdir)/src; d=`pwd`; \
	  for p in `cat INSTALL`; do \
	    echo "Compiling $$p..."; cd $$d/$$p; \
	    $(DESTDIR)$(bindir)/gbc$(GAMBAS_VERSION) -agt -r $(DESTDIR)$(prefix); \
	    if test $$? -eq 0; then \
	      $(DESTDIR)$(bindir)/gba$(GAMBAS_VERSION); \
	      rm -rf .gambas; \
	      echo "Installing $$p..."; \
	      $(INSTALL) $$p.gambas $(DESTDIR)$(bindir); \
	    else \
	      echo "|| Unable to compile $$p" >> ../../../warnings.log; \
	    fi \
	  done)
	##@if test "$(bindir)" != "$(ROOT)/usr/bin" && test "$(bindir)" != "$(ROOT)/usr/bin/"; then
	@$(LN_S) -f gambas$(GAMBAS_VERSION).gambas $(DESTDIR)$(bindir)/gambas$(GAMBAS_VERSION) || true
	@if test x"$(XDG_UTILS)" != x; then \
	  echo "Installing IDE application icon..."; \
	  xdg-icon-resource install --novendor --context apps --size 256 $(srcdir)/desktop/gambas3.png gambas3; \
	  xdg-icon-resource install --novendor --context apps --size 48 $(srcdir)/desktop/gambas3-48.png gambas3; \
	  echo "Installing IDE application menu..."; \
	  $(INSTALL) -d $(DESTDIR)$(datarootdir)/desktop-directories; \
	  xdg-desktop-menu install --novendor $(srcdir)/desktop/gambas3.desktop; \
	  rmdir $(DESTDIR)$(datarootdir)/desktop-directories >/dev/null 2>&1 || true; \
	fi
	
	@echo "Installing the scripter..."
	@$(LN_S) -f gbs$(GAMBAS_VERSION).gambas $(DESTDIR)$(bindir)/gbs$(GAMBAS_VERSION) || true
	@$(LN_S) -f gbs$(GAMBAS_VERSION).gambas $(DESTDIR)$(bindir)/gbw$(GAMBAS_VERSION) || true
	@if test x"$(XDG_UTILS)" != x; then \
	  echo "Registering Gambas script mimetype..."; \
	  xdg-icon-resource install --context mimetypes --size 256 $(srcdir)/mime/application-x-gambasscript.png application-x-gambasscript; \
	  xdg-icon-resource install --context mimetypes --size 48 $(srcdir)/mime/application-x-gambasscript-48.png application-x-gambasscript; \
	  xdg-mime install $(srcdir)/mime/application-x-gambasscript.xml; \
	  echo "Registering Gambas server page mimetype..."; \
	  xdg-icon-resource install --context mimetypes --size 256 $(srcdir)/mime/application-x-gambasserverpage.png application-x-gambasserverpage; \
	  xdg-icon-resource install --context mimetypes --size 48 $(srcdir)/mime/application-x-gambasserverpage-48.png application-x-gambasserverpage; \
	  xdg-mime install $(srcdir)/mime/application-x-gambasserverpage.xml; \
	fi

	@echo "Installing the Gambas appdata file..."
	@$(INSTALL) -d $(DESTDIR)$(datarootdir)/appdata;
	@$(INSTALL) $(srcdir)/desktop/gambas3.appdata.xml $(DESTDIR)$(datarootdir)/appdata;
	@$(INSTALL) -d $(DESTDIR)$(datarootdir)/metainfo;
	@$(INSTALL) $(srcdir)/desktop/gambas3.appdata.xml $(DESTDIR)$(datarootdir)/metainfo;
	
	@echo "Installing the Gambas template projects..."
	@$(INSTALL) -d $(DESTDIR)$(gbdatadir)/template;
	@cp -R $(srcdir)/template/* $(DESTDIR)$(gbdatadir)/template;

uninstall-local:
	@echo "Uninstalling the development environment..."
	@rm -f $(DESTDIR)$(bindir)/gambas$(GAMBAS_VERSION)
	@rm -f $(DESTDIR)$(bindir)/gbs$(GAMBAS_VERSION)
	@rm -f $(DESTDIR)$(bindir)/gbw$(GAMBAS_VERSION)
	@(cd $(srcdir)/src; for p in `cat INSTALL`; do rm -f $(DESTDIR)$(bindir)/$$p.gambas; done)
	@if test x"$(XDG_UTILS)" != x; then \
		xdg-mime uninstall $(srcdir)/mime/application-x-gambasscript.xml; \
		xdg-mime uninstall $(srcdir)/mime/application-x-gambasserverpage.xml; \
		xdg-icon-resource uninstall --context mimetypes --size 256 application-x-gambasscript; \
		xdg-icon-resource uninstall --context mimetypes --size 256 application-x-gambasserverpage; \
		xdg-icon-resource uninstall --context apps --size 256 gambas3; \
	fi
	@rm -rf $(DESTDIR)$(datarootdir)/metainfo/gambas3.appdata.xml
	@rm -rf $(DESTDIR)$(datarootdir)/appdata/gambas3.appdata.xml
	@rm -rf $(DESTDIR)$(gbdatadir)/template
	@rmdir $(DESTDIR)$(datarootdir)/metainfo >/dev/null 2>&1 || true
	@rmdir $(DESTDIR)$(datarootdir)/appdata >/dev/null 2>&1 || true
	@rmdir $(DESTDIR)$(gbdatadir) >/dev/null 2>&1 || true

dist-hook:
	@(cd $(distdir)/src; \
	rm -rf `find . -name ".gambas" -o -name ".action" -o -name ".lock" -o -name ".xvpics" -o -name "*~" -o -name "*.out"  -o -name "*.pot" -o -name "*.gambas" -o -name "core*" -o -name ".kdbg*" -o -name ".svn"`;)

