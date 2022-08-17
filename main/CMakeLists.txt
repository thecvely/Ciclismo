idf_component_register(SRCS "ciclismo.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${project_dir}/html_libs/pure.css
                    EMBED_TXTFILES ${project_dir}/html_libs/root.html
                    EMBED_TXTFILES ${project_dir}/server_certs/hcert.pem
                    EMBED_TXTFILES ${project_dir}/server_certs/hkey.pem
                    EMBED_TXTFILES ${project_dir}/html_libs/sta.html
                    EMBED_TXTFILES ${project_dir}/html_libs/ap.html)
                    
