Import("env")

# Avoid intermittent archive/object write failures on synced folders by
# forcing a single SCons job.
env.SetOption("num_jobs", 1)
