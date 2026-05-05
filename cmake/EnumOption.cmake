# Defines an enum option with specified values and description
macro(enum_option var values description)
  # Set the possible values for the option
  set(${var}_VALUES ${values})

  # Get the first value as the default
  list(GET ${var}_VALUES 0 default)

  # Create a cache variable with the default value and description
  set(${var}
      "${default}"
      CACHE STRING "${description}")

  # Set the possible values as property strings for the cache variable
  set_property(CACHE ${var} PROPERTY STRINGS ${${var}_VALUES})

  # Check if the provided value is within the possible values
  if(NOT ";${${var}_VALUES};" MATCHES ";${${var}};")
    message(
      FATAL_ERROR
        "Unknown value ${${var}}. Only -D${var}=${${var}_VALUES} allowed.")
  endif()
endmacro()
