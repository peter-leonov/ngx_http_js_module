ngx_addon_name=ngx_http_js_module

cat << END                                                >> $NGX_MAKEFILE

test-js:
	@ echo "Testing $ngx_addon_name"
	@ $ngx_addon_dir/run-tests "$ngx_addon_dir" "\$(PWD)/objs"

END


cat << END                                                >> Makefile

test-js:
	\$(MAKE) -f objs/Makefile test-js

END

