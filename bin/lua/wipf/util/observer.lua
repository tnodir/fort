-- WIPF Observer Utilities

local observer = {}

local slots_meta = {__mode = "k"}
local signals = {n = 0}


-- Acquire signal number
function observer.signal()
  local index = signals.n + 1
  local slots = setmetatable({}, slots_meta)

  signals[index], signals.n = slots, index

  return index
end

-- Connect sender's signal to reciever's slot
function observer.connect(signal_index, receiver, slot_function)
  local slots = signals[signal_index]
  local old_slot_function = slots[receiver]

  slots[receiver] = slot_function

  return old_slot_function
end

-- Emit sender's signal to recievers' slots
function observer.emit(signal_index, sender, ...)
  local slots = signals[signal_index]

  for receiver, slot_function in pairs(slots) do
    slot_function(receiver, ...)
  end
end


return observer
