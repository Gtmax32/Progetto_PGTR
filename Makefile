.PHONY: clean All

All:
	@echo "----------Building project:[ Goriziana - Debug ]----------"
	@cd "Goriziana" && "$(MAKE)" -f  "Goriziana.mk" && "$(MAKE)" -f  "Goriziana.mk" PostBuild
clean:
	@echo "----------Cleaning project:[ Goriziana - Debug ]----------"
	@cd "Goriziana" && "$(MAKE)" -f  "Goriziana.mk" clean
