Hey Claude, I am going to take a pause on this project and hand it off to my group members. They are going to open a brand new Claude session to continue the work, and they need to bring the new AI completely up to speed.

Since you have the entire history of our conversation, the exact code structure we decided on, and all the project constraints we've discussed, I want you to write the handoff prompt for them.

Please write a massive, comprehensive prompt that my colleagues can copy and paste directly into their new Claude session. The prompt you generate must include:

    The Role: Tell the new Claude that it is a strict, low-level C coding assistant.

    The Constraints: Explicitly list the project rules we've been following (e.g., strictly POSIX low-level calls like fork/execvp, absolutely no system() or bash, using write() instead of printf for standard output, and preparing for the Advanced Phase redirections).

    The 'No IPC' Rule: Make it crystal clear that we have not learned Named Pipes (FIFOs) yet, so the new Claude must NOT generate any pipe code. It should only use /* TODO (IPC) */ comments for those parts.

    The Current Status: Summarize exactly how we built runner.c and scheduler.h/c (mentioning that we avoided function pointers for the scheduler to keep it readable).

    The Immediate Next Step: Give the new Claude its first task: generating the skeleton for controller.c (parsing <parallel-commands> and <sched-policy>, initializing the scheduler, and setting up the main loop).

    Code Placeholders: Leave a clear instruction at the very bottom of the prompt telling my colleagues exactly where to paste runner.c, scheduler.h, and scheduler.c.

Write this from the perspective of a developer handing over a project, so the new Claude knows exactly what to do.