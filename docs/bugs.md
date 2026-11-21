# Bugs hit and how they were found

Template, one section per bug. Aim for 2-3 real ones; this is your interview material.

## Bug N: <short title>
- **Symptom:** what you saw (UART output, hang, wrong value)
- **Trace before:** `docs/traces/<file>` (register access log / decoded transaction)
- **Hypothesis -> test:** what you suspected and how you checked (GDB, register log)
- **Root cause:** cite the datasheet / RM section that you had misread
- **Fix:** commit + one-line description
- **Trace after:** `docs/traces/<file>`
- **Regression test:** which Robot test now covers it
