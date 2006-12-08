=begin
  Copyright 2006 Suraj N. Kurapati

  This file is part of Ruby-VPI.

  Ruby-VPI is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  Ruby-VPI is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Ruby-VPI; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
=end

module RubyVpi
  # Initializes the bench by loading:
  # 1. the design.rb file
  # 2. the proto.rb file if prototyping is enabled
  # 3. the spec.rb file
  #
  # aDesignId:: The name of the Ruby design object.
  # aSpecFormat:: The format being used by the specification.
  def RubyVpi.init_bench aDesignId, aSpecFormat
    if caller.find {|s| s =~ /^(.*?)_bench.rb:/}
      testName = $1
    else
      raise 'Unable to determine name of test.'
    end

    # set up the VPI utility layer
      require 'ruby-vpi/vpi'

      Object.class_eval do
        include Vpi
      end

    # set up the specification library
      case aSpecFormat
        when :xUnit
          require 'test/unit'

        when :rSpec
          ARGV.concat %w[-f s]
          require 'ruby-vpi/rspec'

        when :tSpec
          ARGV << '-rs'
          require 'test/spec'
      end

    # service the $ruby_init() task
      relay_verilog

    # set up code coverage analysis
      unless (ENV['COVERAGE'] || '').empty?
        require 'ruby-vpi/rcov'

        RubyVpi.with_coverage_analysis do |a|
          a.dump_coverage_info [
            Rcov::TextReport.new,
            Rcov::HTMLCoverage.new(:destdir => "#{testName}_coverage")
          ]
        end
      end

    # load the design under test
      unless design = vpi_handle_by_name("#{testName}_bench", nil)
        raise "Verilog bench for test #{testName.inspect} is inaccessible."
      end

      Kernel.const_set(aDesignId, design)
      require "#{testName}_design.rb"

    # load the design's prototype
      unless (ENV['PROTOTYPE'] || '').empty?
        require "#{testName}_proto.rb"

        Vpi.class_eval do
          define_method :relay_verilog do
            design.simulate!
          end
        end

        puts "#{Config::PROJECT_NAME}: prototype has been enabled for test #{testName.inspect}"
      end

    # load the design's specification
      require "#{testName}_spec.rb"
  end

  # Provides information about this project's configuration.
  module Config
    PROJECT_ID = 'ruby-vpi'
    PROJECT_NAME = 'Ruby-VPI'
    PROJECT_URL = "http://#{PROJECT_ID}.rubyforge.org"
    WEBSITE_URL = PROJECT_URL + "/doc"
    PROJECT_SUMMARY = "Ruby interface to IEEE 1364-2005 Verilog VPI"
    PROJECT_DETAIL = "#{PROJECT_NAME} is a #{PROJECT_SUMMARY}. It lets you create complex Verilog test benches easily and wholly in Ruby."

    Simulator = Struct.new(:id, :name, :compiler_args, :linker_args)

    # List of supported Verilog simulators.
    SIMULATORS = [
      Simulator.new(:cver, 'GPL Cver', '-DPRAGMATIC_CVER', ''),
      Simulator.new(:ivl, 'Icarus Verilog', '-DICARUS_VERILOG', ''),
      Simulator.new(:vcs, 'Synopsys VCS', '-DSYNOPSYS_VCS', ''),
      Simulator.new(:vsim, 'Mentor Modelsim', '-DMENTOR_MODELSIM', ''),
    ]
  end
end
