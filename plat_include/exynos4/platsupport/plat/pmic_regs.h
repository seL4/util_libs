/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


/* All general IO ports and busses */
#define PMICREG_VDDIO      PMICREG_VDDOUT03
/* Signal voltage for SD card */
#define PMICREG_VDDMMC2    PMICREG_VDDOUT04
/* Signal voltages for other SD/EMMC ports */
#define PMICREG_VDDMMC013  PMICREG_VDDOUT05
/* Voltage to eMMC expansion port */
#define PMICREG_VDDEMMC    PMICREG_VDDOUT20
/* Voltage to SD card */
#define PMICREG_TFLASH     PMICREG_VDDOUT21
/* This pin connected directly to LCD header on base board */
#define PMICREG_VDDQLCD    PMICREG_VDDOUT25
/* Hold reset button for X seconds to reboot */
#define PMICREG_RSTDELAY (PMICREG_4 + 6)

#define PMIC_1V8_EN 0x14


/************************
 **** PMIC REGISTERS ****
 ************************/
#define PMICREG_CHIPID 0x00 /*  4 bytes */
#define PMICREG_4      0x04 /* 60 bytes */
#define RSTD_3SEC 0x08
#define RSTD_7SEC /* 0x08? (default)*/


/* Output control 1 */
#define PMICREG_VDDOUT01 0x40 /*  1 byte  */
#define PMICREG_VDDOUT02 0x41 /*  1 byte  */
#define PMICREG_VDDOUT03 0x42 /*  1 byte  */
#define PMICREG_VDDOUT04 0x43 /*  1 byte  */
#define PMICREG_VDDOUT05 0x44 /*  1 byte  */
#define PMICREG_VDDOUT06 0x45 /*  1 byte  */
#define PMICREG_VDDOUT07 0x46 /*  1 byte  */
#define PMICREG_VDDOUT08 0x47 /*  1 byte  */
#define PMICREG_VDDOUT09 0x48 /*  1 byte  */
#define PMICREG_VDDOUT10 0x49 /*  1 byte  */
#define PMICREG_VDDOUT11 0x4a /*  1 byte  */
#define PMICREG_VDDOUT12 0x4b /*  1 byte  */
#define PMICREG_VDDOUT13 0x4c /*  1 byte  */
#define PMICREG_VDDOUT14 0x4d /*  1 byte  */
#define PMICREG_VDDOUT15 0x4e /*  1 byte  */
#define PMICREG_VDDOUT16 0x4f /*  1 byte  */
#define PMICREG_VDDOUT17 0x50 /*  1 byte  */
#define PMICREG_VDDOUT18 0x51 /*  1 byte  */
#define PMICREG_VDDOUT19 0x52 /*  1 byte  */
#define PMICREG_VDDOUT20 0x53 /*  1 byte  */
#define PMICREG_VDDOUT21 0x54 /*  1 byte  */
#define PMICREG_VDDOUT22 0x55 /*  1 byte  */
#define PMICREG_VDDOUT23 0x56 /*  1 byte  */
#define PMICREG_VDDOUT24 0x57 /*  1 byte  */
#define PMICREG_VDDOUT25 0x58 /*  1 byte  */
#define PMICREG_VDDOUT26 0x59 /*  1 byte  */

/* Output control 2 */
#define PMICREG_96     0x60 /*  1 byte  */
#define PMICREG_97       97 /*  1 byte  */
#define PMICREG_98       98 /*  1 byte  */
#define PMICREG_99       99 /*  1 byte  */
#define PMICREG_100     100 /*  1 byte  */
#define PMICREG_101     101 /*  1 byte  */
#define PMICREG_102     102 /*  1 byte  */
#define PMICREG_103     103 /*  1 byte  */
#define PMICREG_104     104 /*  1 byte  */
#define PMICREG_105     105 /*  1 byte  */
#define PMICREG_106     106 /*  1 byte  */
#define PMICREG_107     107 /*  1 byte  */
#define PMICREG_108     108 /*  1 byte  */
#define PMICREG_109     109 /*  1 byte  */
#define PMICREG_110     110 /*  1 byte  */
#define PMICREG_111     111 /*  1 byte  */
#define PMICREG_112     112 /*  1 byte  */
#define PMICREG_113     113 /*  1 byte  */
#define PMICREG_114     114 /*  1 byte  */
#define PMICREG_115     115 /*  1 byte  */
#define PMICREG_116     116 /*  1 byte  */
#define PMICREG_117     117 /*  1 byte  */
#define PMICREG_118     118 /*  1 byte  */
#define PMICREG_119     119 /*  1 byte  */
#define PMICREG_120     120 /*  1 byte  */

#define PMICREG_121     121 /*  6 bytes */
/**********************/


/**********************/
#define PMICREG_128     128 /*  2 bytes */
#define PMICREG_130     130 /*  1 byte  */
#define PMICREG_131     131 /*  1 byte  */
#define PMICREG_132     132 /*  1 byte  */
#define PMICREG_133     133 /*  1 byte  */
#define PMICREG_134     134 /*  1 byte  */
#define PMICREG_135     135 /*  3 bytes */
#define PMICREG_138     138 /*  1 bytes */
#define PMICREG_139     139 /*  1 bytes */
#define PMICREG_140     140 /*  1 bytes */
#define PMICREG_141     141 /*  1 bytes */
#define PMICREG_142     142 /*  1 bytes */
#define PMICREG_143     143 /*  1 bytes */
#define PMICREG_144     144 /*  1 bytes */
#define PMICREG_145     145 /*  1 bytes */
#define PMICREG_146     146 /*  1 bytes */
#define PMICREG_147     147 /*  1 bytes */
#define PMICREG_148     148 /*  1 bytes */
#define PMICREG_149     149 /*  2 bytes */
#define PMICREG_151     151 /*  1 byte  */
#define PMICREG_152     152 /*  1 byte  */
#define PMICREG_153     153 /*  1 byte  */
#define PMICREG_154     154 /*  1 byte  */
#define PMICREG_155     155 /*  1 byte  */
#define PMICREG_156     156 /*  1 byte  */
#define PMICREG_157     157 /*  1 byte  */
#define PMICREG_158     158 /*  1 byte  */
#define PMICREG_159     159 /*  1 byte  */
#define PMICREG_160     160 /*  1 byte  */
#define PMICREG_161     161 /*  1 byte  */
#define PMICREG_162     162 /*  1 byte  */
#define PMICREG_163     163 /*  1 byte  */
#define PMICREG_164     164 /*  1 byte  */
#define PMICREG_165     165 /*  1 byte  */
#define PMICREG_166     166 /*  1 byte  */
#define PMICREG_167     167 /*  1 byte  */
#define PMICREG_168     168 /*  1 byte  */
#define PMICREG_169     169 /*  1 byte  */
#define PMICREG_170     170 /*  1 byte  */
#define PMICREG_171     171 /*  1 byte  */
#define PMICREG_172     172 /*  1 byte  */
#define PMICREG_173     173 /*  1 byte  */
#define PMICREG_174     174 /*  1 byte  */
#define PMICREG_175     175 /*  1 byte  */
#define PMICREG_176     176 /*  1 byte  */
#define PMICREG_177     177 /*  1 byte  */
#define PMICREG_178     178 /*  1 byte  */
#define PMICREG_179     179 /*  1 byte  */
#define PMICREG_180     180 /*  1 byte  */
#define PMICREG_181     181 /*  1 byte  */
#define PMICREG_182     182 /*  1 byte  */
#define PMICREG_183     183 /*  1 byte  */
#define PMICREG_184     184 /*  1 byte  */
#define PMICREG_185     185 /*  1 byte  */
#define PMICREG_186     186 /*  1 byte  */
#define PMICREG_187     187 /*  1 byte  */
#define PMICREG_188     188 /*  1 byte  */
#define PMICREG_189     189 /*  1 byte  */
#define PMICREG_190     190 /*  1 byte  */
#define PMICREG_191     191 /*  1 byte  */
#define PMICREG_192     192 /*  1 byte  */
#define PMICREG_193     193 /*  1 byte  */
#define PMICREG_194     194 /*  1 byte  */
#define PMICREG_195     195 /*  1 byte  */
#define PMICREG_196     196 /*  1 byte  */
#define PMICREG_197     197 /*  1 byte  */
#define PMICREG_198     198 /*  1 byte  */
#define PMICREG_199     199 /*  1 byte  */
#define PMICREG_200     200 /*  1 byte  */
#define PMICREG_201     201 /*  1 byte  */
#define PMICREG_202     202 /*  1 byte  */
#define PMICREG_203     203 /*  1 byte  */
#define PMICREG_204     204 /*  1 byte  */
#define PMICREG_205     205 /*  1 byte  */
#define PMICREG_206     206 /*  1 byte  */
#define PMICREG_207     207 /*  1 byte  */
#define PMICREG_208     208 /*  1 byte  */
#define PMICREG_209     209 /*  1 byte  */
#define PMICREG_210     210 /*  1 byte  */
#define PMICREG_211     211 /*  1 byte  */
#define PMICREG_212     212 /*  1 byte  */
#define PMICREG_213     213 /*  1 byte  */
#define PMICREG_214     214 /*  1 byte  */
#define PMICREG_215     215 /*  1 byte  */
#define PMICREG_216     216 /*  1 byte  */
#define PMICREG_217     217 /*  1 byte  */
#define PMICREG_218     218 /*  1 byte  */
#define PMICREG_219     219 /*  1 byte  */
#define PMICREG_220     220 /*  1 byte  */
#define PMICREG_221     221 /*  1 byte  */
#define PMICREG_222     222 /*  1 byte  */
#define PMICREG_223     223 /*  1 byte  */
#define PMICREG_224     224 /*  1 byte  */
#define PMICREG_225     225 /*  1 byte  */
#define PMICREG_226     226 /*  1 byte  */
#define PMICREG_227     227 /*  1 byte  */
#define PMICREG_228     228 /*  1 byte  */
#define PMICREG_229     229 /*  1 byte  */
#define PMICREG_230     230 /*  1 byte  */
#define PMICREG_231     231 /*  1 byte  */
#define PMICREG_232     232 /*  1 byte  */
#define PMICREG_233     233 /*  1 byte  */
#define PMICREG_234     234 /*  1 byte  */
#define PMICREG_235     235 /*  1 byte  */
#define PMICREG_236     236 /*  1 byte  */
#define PMICREG_237     237 /*  1 byte  */
#define PMICREG_238     238 /*  1 byte  */
#define PMICREG_239     239 /*  1 byte  */
#define PMICREG_240     240 /*  1 byte  */
#define PMICREG_241     241 /*  1 byte  */
#define PMICREG_242     242 /*  1 byte  */
#define PMICREG_243     243 /*  1 byte  */
#define PMICREG_244     244 /*  1 byte  */
#define PMICREG_245     245 /*  1 byte  */
#define PMICREG_246     246 /*  1 byte  */
#define PMICREG_247     247 /*  1 byte  */
#define PMICREG_248     248 /*  1 byte  */
#define PMICREG_249     249 /*  1 byte  */
#define PMICREG_250     250 /*  1 byte  */
#define PMICREG_251     251 /*  1 byte  */
#define PMICREG_252     252 /*  3 bytes */
#define PMICREG_255     255 /*  1 byte  */

