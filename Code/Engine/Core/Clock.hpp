//--------------------------------------------------------------------------------------------------
// Timer class that can be attached to any clock in a hierarchy and correctly handles duration
// regardless of update frequency.
class Timer
{
public:
    // Create a clock with a period and the the specified clock. If the clock
    // is null, use the system clock.
    explicit Timer(double period, const Clock* clock = nullptr);

    // Set the start time to the clock's current total time.
    void Start();

    // Set the start time back to negative one.
    void Stop();

    // Returns zero if stopped, otherwise returns the time elapsed between the clock's current
    // time and our start time.
    double GetElapsedTime() const;

    // Return the elapsed time as a percentage of our period. This can be greater than 1.
    double GetElapsedFraction() const;

    // Returns true if our start time less than zero.
    bool IsStopped() const;

    // Returns true if our elapsed time is greater than our period and we are not stopped.
    bool HasPeriodElapsed() const;

    // If a period has elapsed and we are not stopped, decrements a period by adding a
    // period to the start time and returns true. Generally called within a loop until it
    // returns false so the caller can process each elapsed period.
    bool DecrementPeriodIfElapsed();

    // The clock to use for getting the current time.
    const Clock* m_clock = nullptr;

    // Clock time at which the timer was started. This is incremented by one period each
    // time we decrement a period, however, so it is not always the original start time.
    // A value of -1.0f indicates the timer is stopped.
    double m_startTime = -1.0f;

    // The time interval it takes for a period to elapse.
    double m_period = 0.0f;
};
