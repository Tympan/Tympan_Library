//These filter coefficients were written by the Matlab code shown at the bottom of this file.
//Filter order: 3
//Cutoff Frequencies: 700  2400
 
#define SOS_ASSUMED_SAMPLE_RATE_HZ 24000
#define SOS_N_FILTERS 3
#define SOS_N_BIQUADS_PER_FILTER 3
float32_t all_matlab_sos_delay_msec[SOS_N_FILTERS] = {0.000, 0.644, 1.000};
float32_t all_matlab_sos[SOS_N_FILTERS][SOS_N_BIQUADS_PER_FILTER*6] = {  
    { 
       -4.2218239468e-07, -8.4686038350e-07, -4.2468291892e-07, 1.0000000000e+00, -1.6721652534e+00, 7.0064267213e-01, \
       1.0000000000e+00, 1.9999692783e+00, 9.9998086545e-01, 1.0000000000e+00, -1.7420316577e+00, 7.7169891991e-01, \
       1.0000000000e+00, 1.9941195464e+00, 9.9413104295e-01, 1.0000000000e+00, -1.8779350827e+00, 9.0991681629e-01, \
    }, 
    { 
       7.3956888131e-03, -1.9190456802e-10, -7.3957832466e-03, 1.0000000000e+00, -1.5363890805e+00, 6.3095301138e-01, \
       1.0000000000e+00, 1.9999936286e+00, 9.9999362867e-01, 1.0000000000e+00, -1.4235418502e+00, 7.2139253737e-01, \
       1.0000000000e+00, -1.9999936027e+00, 9.9999360272e-01, 1.0000000000e+00, -1.8597951696e+00, 8.9513594272e-01, \
    }, 
    { 
       -2.8940691708e-01, 5.8052456917e-01, -2.9112103168e-01, 1.0000000000e+00, -1.0320694053e+00, 2.7570794247e-01, \
       1.0000000000e+00, -1.9999692783e+00, 9.9998086545e-01, 1.0000000000e+00, -1.1429805025e+00, 4.1280159810e-01, \
       1.0000000000e+00, -1.9941195464e+00, 9.9413104296e-01, 1.0000000000e+00, -1.4043848905e+00, 7.3591519120e-01, \
    } 
  };
 
/* The matlab code that created this file is below

function makeIIRFilterbank()

fs_Hz = 24000; %assumed sample rate
N_IIR=3;  %desired filter order
%cutoff_Hz=[317.2,503.0,797.6,1265,2006,3181,5045]; % crossover frequencies
cutoff_Hz=[700,2400]; % crossover frequencies
use_delays = 1;  %1 = use delays to better align individual filters, 0 = defeat.

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
*/
