# Ruby prototype of the design under test's Verilog implementation.
def feign!
  if <%= aModuleInfo.clock_port.name %>.posedge?
    # discard old outputs
    <% aModuleInfo.output_ports.each do |port| %>
      <%= port.name %>.x!
    <% end %>

    # process new inputs

    # produce new outputs
  end
end
