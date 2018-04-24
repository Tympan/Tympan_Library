function makeIIRFilterbank()

fs_Hz = 24000; %assumed sample rate
N_IIR=3;  %desired filter order
cutoff_Hz=[317.2,503.0,797.6,1265,2006,3181,5045]; % crossover frequencies
use_delays = 0;  %1 = use delays to better align individual filters, 0 = defeat.

%compute all of the filter coefficients
all_sos={};
all_gd_msec = zeros(length(cutoff_Hz)+1,1);
N_gd = 100;
for I=1:length(cutoff_Hz)+1
    if I==1
        [b,a] = butter(2*N_IIR,cutoff_Hz(1)/(fs_Hz/2));          % low-pass
        f_Hz = [0:1/N_gd:1]*(cutoff_Hz(1)-50) + 50;
    elseif I==length(cutoff_Hz)+1
        [b,a] = butter(2*N_IIR,cutoff_Hz(end)/(fs_Hz/2),'high'); %high-pass
        f_Hz = [0:1/N_gd:1]*(0.9*(fs_Hz/2)-cutoff_Hz(end)) + cutoff_Hz(end);
    else
        bp_Hz = [cutoff_Hz(I-1) cutoff_Hz(I)];
        [b,a] = butter(N_IIR,bp_Hz/(fs_Hz/2)); %band-pass
        f_Hz = [0:1/N_gd:1]*(cutoff_Hz(I)-cutoff_Hz(I-1))+cutoff_Hz(I-1);
    end
       
    if (use_delays==1);
        %flip the phase of the low-pass and high-pass filters, but not the BP filters in the middle
        if ((I==1) || (I==(length(cutoff_Hz)+1)));  b = -b; end
        
        %calculate the mean delay of this filter
        all_gd_msec(I) = 1000*(mean(grpdelay(b,a,f_Hz,fs_Hz))/fs_Hz);
    end;
    
    %use matlab's conversion to second-order sections, no gain term
    sos=tf2sos(b,a);
    
    %accumulate the sos representations
    all_sos{I} = sos;
end

%align all of the filter delays to the one with the longest delay
all_gd_msec = max(all_gd_msec) - all_gd_msec;

%% print text to screen for copying into C++
diary off
!del filter_coeff_sos.h
diary('filter_coeff_sos.h')
disp(['//These filter coefficients were written by the Matlab code shown at the bottom of this file.']);
disp(['//Filter order: ' num2str(N_IIR)]);
disp(['//Cutoff Frequencies: ' num2str(cutoff_Hz)]);
disp([' ']);
disp(['#define SOS_ASSUMED_SAMPLE_RATE_HZ ' num2str(fs_Hz)]);
disp(['#define SOS_N_FILTERS ' num2str(length(all_sos))]);
disp(['#define SOS_N_BIQUADS_PER_FILTER ' num2str(size(all_sos{1},1))]);
foo=sprintf('%5.3f, ', all_gd_msec); foo = foo(1:end-2);
disp(['float32_t all_matlab_sos_delay_msec[SOS_N_FILTERS] = {' foo '};']);
disp(['float32_t all_matlab_sos[SOS_N_FILTERS][SOS_N_BIQUADS_PER_FILTER*6] = {  ']);
for Ifilt=1:length(all_sos)
    sos = all_sos{Ifilt};
    disp(['    { ']);
    for I=1:size(sos,1)
        disp(sprintf('       %.10e, %.10e, %.10e, %.10e, %.10e, %.10e, \\',sos(I,:)));
    end
    
    if Ifilt < length(all_sos)
        disp(['    }, ']);
    else
        disp(['    } ']);
    end
end
disp(['  };']);
disp([' ']);
disp(['/' '*' ' The matlab code that created this file is below']);
eval(['type ' mfilename]);
disp(['*' '/']);
diary off

return

