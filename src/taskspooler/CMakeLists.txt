project(taskspooler)

set(tsp_SRC
    main.c
    server.c
    server_start.c
    client.c
    msgdump.c
    jobs.c
    execute.c
    msg.c
    mail.c
    error.c
    signals.c
    list.c
    print.c
    info.c
    env.c
    tail.c
)


add_executable(tsp ${tsp_SRC})
