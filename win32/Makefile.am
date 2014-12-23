## Automake template for Windows distribution.

ROOT = $(top_srcdir)

EXE = $(ROOT)/src/ifm.exe
LIB = $(ROOT)/lib
IFM = $(ROOT)/doc/examples
DOC = README ../COPYING

DIR = ifm-$(VERSION)
ZIP = $(DIR).zip

ZIPCMD = 7z -r a

zip: $(DIR)
	$(ZIPCMD) $(ZIP) $(DIR)
	rm -rf $(DIR)

$(DIR): exe lib examples doc

exe:
	mkdir -p $(DIR)
	cp $(EXE) $(DIR)

lib:
	mkdir -p $(DIR)/ifm
	cp $(LIB)/*.ifm $(DIR)/ifm
	cp $(LIB)/*.txt $(DIR)/ifm
	cp $(LIB)/*.ps $(DIR)/ifm

examples:
	mkdir -p $(DIR)/examples
	cp $(IFM)/*.ifm $(DIR)/examples

doc:
	cp $(DOC) $(DIR)

clean-local:; rm -rf $(DIR) $(ZIP)
