public interface IMidiEvents
{
    /// Invoked when a Note On event occurs.
    /// Arguments: Channel (0-15), Note Number (0-127), Velocity (0-127).
    event Action<int, int, int> OnNoteOn;

    /// Invoked when a Note Off event occurs.
    /// Arguments: Channel (0-15), Note Number (0-127), Velocity (0-127).
    event Action<int, int, int> OnNoteOff;

    /// Invoked when a Control Change event occurs.
    /// Arguments: Channel (0-15), Controller Number (0-127), Value (0-127).
    event Action<int, int, int> OnControlChange;

    /// Invoked when a Program Change event occurs.
    /// Arguments: Channel (0-15), Program Number (0-127).
    event Action<int, int> OnProgramChange;

    /// Invoked when a Pitch Bend event occurs.
    /// Arguments: Channel (0-15), Value (0-16383).
    event Action<int, int> OnPitchBend;

    /// Invoked when the playback time is updated.
    /// Arguments: Time in seconds.
    event Action<float> OnTime;
}
