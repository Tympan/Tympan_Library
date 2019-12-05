//These filter coefficients were written by the Matlab code shown at the bottom of this file.
//Filter order: 3
//Cutoff Frequencies: 317.2            503          797.6           1265           2006           3181           5045
 
#define SOS_ASSUMED_SAMPLE_RATE_HZ 24000
#define SOS_N_FILTERS 8
#define SOS_N_BIQUADS_PER_FILTER 3
float32_t all_matlab_sos_delay_msec[SOS_N_FILTERS] = {1.597, 0.000, 1.492, 2.435, 3.028, 3.402, 3.638, 3.924};
float32_t all_matlab_sos[SOS_N_FILTERS][SOS_N_BIQUADS_PER_FILTER*6] = {  
    { 
       -4.3795680416e-09, -8.7850244775e-09, -4.4055075790e-09, 1.0000000000e+00, -1.8452635209e+00, 8.5164441591e-01, \
       1.0000000000e+00, 1.9999692783e+00, 9.9998086545e-01, 1.0000000000e+00, -1.8826835085e+00, 8.8919380158e-01, \
       1.0000000000e+00, 1.9941195464e+00, 9.9413104295e-01, 1.0000000000e+00, -1.9512184122e+00, 9.5796569798e-01, \
    }, 
    { 
       1.3711430905e-05, -2.0283434308e-12, -1.3711292683e-05, 1.0000000000e+00, -1.9418353062e+00, 9.5250347954e-01, \
       1.0000000000e+00, 2.0000051143e+00, 1.0000051144e+00, 1.0000000000e+00, -1.9551862528e+00, 9.7128925105e-01, \
       1.0000000000e+00, -2.0000049664e+00, 1.0000049664e+00, 1.0000000000e+00, -1.9734517945e+00, 9.8068716426e-01, \
    }, 
    { 
       5.3168748143e-05, -8.5375722505e-12, -5.3169374853e-05, 1.0000000000e+00, -1.8992739065e+00, 9.2570221606e-01, \
       1.0000000000e+00, 1.9999941867e+00, 9.9999418674e-01, 1.0000000000e+00, -1.9148395554e+00, 9.5491393840e-01, \
       1.0000000000e+00, -1.9999940261e+00, 9.9999402617e-01, 1.0000000000e+00, -1.9514448920e+00, 9.6952041083e-01, \
    }, 
    { 
       2.0339178407e-04, -3.2659637706e-11, -2.0339418149e-04, 1.0000000000e+00, -1.8196863574e+00, 8.8455414845e-01, \
       1.0000000000e+00, 1.9999941867e+00, 9.9999418674e-01, 1.0000000000e+00, -1.8306128212e+00, 9.2961901298e-01, \
       1.0000000000e+00, -1.9999940261e+00, 9.9999402617e-01, 1.0000000000e+00, -1.9070155806e+00, 9.5196087823e-01, \
    }, 
    { 
       7.5854625794e-04, -1.9682884939e-11, -7.5855594361e-04, 1.0000000000e+00, -1.6659447209e+00, 8.2265226162e-01, \
       1.0000000000e+00, 1.9999936286e+00, 9.9999362867e-01, 1.0000000000e+00, -1.6505023005e+00, 8.9153468326e-01, \
       1.0000000000e+00, -1.9999936027e+00, 9.9999360272e-01, 1.0000000000e+00, -1.8135981551e+00, 9.2443891372e-01, \
    }, 
    { 
       2.7360776473e-03, -4.3934569484e-10, -2.7361098980e-03, 1.0000000000e+00, -1.3636204554e+00, 7.3155438551e-01, \
       1.0000000000e+00, 1.9999941867e+00, 9.9999418674e-01, 1.0000000000e+00, -1.2663835045e+00, 8.3658805261e-01, \
       1.0000000000e+00, -1.9999940261e+00, 9.9999402617e-01, 1.0000000000e+00, -1.6123585489e+00, 8.8099099491e-01, \
    }, 
    { 
       9.4242118217e-03, -1.5132929050e-09, -9.4243229068e-03, 1.0000000000e+00, -7.8247426193e-01, 6.0133580902e-01, \
       1.0000000000e+00, 1.9999941867e+00, 9.9999418674e-01, 1.0000000000e+00, -4.9363941852e-01, 7.6537275441e-01, \
       1.0000000000e+00, -1.9999940261e+00, 9.9999402617e-01, 1.0000000000e+00, -1.1829125763e+00, 8.1030351433e-01, \
    }, 
    { 
       -5.9464718481e-02, 1.1933825484e-01, -5.9874474602e-02, 1.0000000000e+00, -2.5561514545e-01, 3.3114880357e-02, \
       1.0000000000e+00, -1.9999799335e+00, 9.9999564253e-01, 1.0000000000e+00, -2.9365458597e-01, 1.8685816495e-01, \
       1.0000000000e+00, -1.9931451012e+00, 9.9316074160e-01, 1.0000000000e+00, -3.9563060245e-01, 5.9901269467e-01, \
    } 
  };
 
/* The matlab code that created this file is below

function makeIIRFilterbank()

fs_Hz = 24000; %assumed sample rate
N_IIR=3;  %desired filter order
cutoff_Hz=[317.2,503.0,797.6,1265,2006,3181,5045]; % crossover frequencies
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
